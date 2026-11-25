package com.xiaoboluo.d2r;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.os.CancellationSignal;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.ParcelFileDescriptor;
import android.os.ProxyFileDescriptorCallback;
import android.os.storage.StorageManager;
import android.provider.DocumentsContract.Document;
import android.provider.DocumentsContract.Root;
import android.provider.DocumentsProvider;
import android.system.ErrnoException;
import android.system.OsConstants;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;




public final class QaDocs extends DocumentsProvider {
    static final String LOCAL = "local", REMOTE = "remote";
    private static final String[] ROOT_COLUMNS = {Root.COLUMN_ROOT_ID, Root.COLUMN_DOCUMENT_ID, Root.COLUMN_TITLE, Root.COLUMN_FLAGS};
    private static final String[] DOC_COLUMNS = {Document.COLUMN_DOCUMENT_ID, Document.COLUMN_DISPLAY_NAME, Document.COLUMN_SIZE, Document.COLUMN_MIME_TYPE, Document.COLUMN_FLAGS};
    private HandlerThread proxyThread;

    @Override public boolean onCreate() { return true; }

    private File dir() { return new File(getContext().getFilesDir(), "qadocs"); }

    private File file(String documentId) throws FileNotFoundException {
        int colon = documentId.indexOf(':');
        if (colon < 0) throw new FileNotFoundException("no root in " + documentId);
        String root = documentId.substring(0, colon), name = documentId.substring(colon + 1);
        if (!root.equals(LOCAL) && !root.equals(REMOTE)) throw new FileNotFoundException("unknown root " + root);
        if (name.isEmpty() || name.contains("/") || name.startsWith(".")) throw new FileNotFoundException("bad name " + name);
        File f = new File(dir(), name);
        if (!f.isFile()) throw new FileNotFoundException(name);
        return f;
    }

    @Override public Cursor queryRoots(String[] projection) {
        MatrixCursor c = new MatrixCursor(projection != null ? projection : ROOT_COLUMNS);
        for (String root : new String[]{LOCAL, REMOTE})
            c.newRow().add(Root.COLUMN_ROOT_ID, root).add(Root.COLUMN_DOCUMENT_ID, root + ":").add(Root.COLUMN_TITLE, "qa " + root).add(Root.COLUMN_FLAGS, Root.FLAG_LOCAL_ONLY);
        return c;
    }

    @Override public Cursor queryDocument(String documentId, String[] projection) throws FileNotFoundException {
        MatrixCursor c = new MatrixCursor(projection != null ? projection : DOC_COLUMNS);
        File f = file(documentId);
        c.newRow().add(Document.COLUMN_DOCUMENT_ID, documentId).add(Document.COLUMN_DISPLAY_NAME, f.getName()).add(Document.COLUMN_SIZE, f.length())
            .add(Document.COLUMN_MIME_TYPE, "application/octet-stream").add(Document.COLUMN_FLAGS, 0);
        return c;
    }

    @Override public Cursor queryChildDocuments(String parentDocumentId, String[] projection, String sortOrder) {
        return new MatrixCursor(projection != null ? projection : DOC_COLUMNS);
    }

    @Override public ParcelFileDescriptor openDocument(String documentId, String mode, CancellationSignal signal) throws FileNotFoundException {
        if (!"r".equals(mode)) throw new FileNotFoundException("read only");
        File f = file(documentId);
        if (documentId.startsWith(LOCAL + ":")) return ParcelFileDescriptor.open(f, ParcelFileDescriptor.MODE_READ_ONLY);
        RandomAccessFile in = new RandomAccessFile(f, "r");
        synchronized (this) {
            if (proxyThread == null) { proxyThread = new HandlerThread("qa-docs-proxy"); proxyThread.start(); }
        }
        try {
            return getContext().getSystemService(StorageManager.class).openProxyFileDescriptor(ParcelFileDescriptor.MODE_READ_ONLY, new ProxyFileDescriptorCallback() {
                @Override public long onGetSize() { return f.length(); }
                @Override public int onRead(long offset, int size, byte[] data) throws ErrnoException {
                    try { in.seek(offset); int n = in.read(data, 0, size); return Math.max(0, n); }
                    catch (IOException e) { throw new ErrnoException("read", OsConstants.EIO); }
                }
                @Override public void onRelease() { try { in.close(); } catch (IOException e) { AppArt.log("qa docs: proxy close " + e); } }
            }, new Handler(proxyThread.getLooper()));
        } catch (IOException e) {
            try { in.close(); } catch (IOException closing) { e.addSuppressed(closing); }
            throw new FileNotFoundException("proxy " + e);
        }
    }
}
