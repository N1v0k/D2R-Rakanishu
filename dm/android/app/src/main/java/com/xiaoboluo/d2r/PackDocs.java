package com.xiaoboluo.d2r;







final class PackDocs {
    static final String EXTERNAL_STORAGE = "com.android.externalstorage.documents", DOWNLOADS = "com.android.providers.downloads.documents";

    private PackDocs() {}


    static boolean local(String authority, String documentId, String qaAuthority, String qaRoot) {
        if (authority == null || documentId == null) return false;
        if (authority.equals(EXTERNAL_STORAGE) || authority.equals(DOWNLOADS)) return true;
        return qaAuthority != null && authority.equals(qaAuthority) && documentId.startsWith(qaRoot + ":");
    }


    static boolean descriptor(boolean regular, long end, long statSize, long reported) {
        return regular && reported > 0 && end == statSize && end == reported;
    }
}
