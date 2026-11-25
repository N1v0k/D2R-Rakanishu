package com.xiaoboluo.d2r;

import android.app.Activity;
import android.os.SystemClock;
import android.text.InputType;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.SurroundingText;
import android.view.inputmethod.TextAttribute;






final class SearchTyping {
    static final long CARET_MS = 500;

    private final View view;
    private final Runnable searched, changed;
    private final RecipeSearchInput input = new RecipeSearchInput();


    private boolean editing, imeSeen, batchChanged;
    private int editGen, batch;
    private float imeTop = Float.MAX_VALUE;
    private final Runnable caret = this::caretTick, showIme = this::showIme;

    SearchTyping(View view, Runnable searched, Runnable changed) {
        this.view = view;
        this.searched = searched;
        this.changed = changed;
    }

    boolean editing() { return editing; }
    String text() { return input.text(); }
    boolean isEmpty() { return input.isEmpty(); }
    float imeTop() { return imeTop; }

    boolean caretOn() { return editing && (SystemClock.uptimeMillis() / CARET_MS) % 2 == 0; }


    void setText(String s) {
        input.clear();
        if (!s.isEmpty()) input.commit(s);
        if (editing) { InputMethodManager imm = imm(); if (imm != null) imm.restartInput(view); }
    }

    private InputMethodManager imm() { return view.getContext().getSystemService(InputMethodManager.class); }



    void begin() {
        if (editing) return;
        if (view.getContext() instanceof Activity) ((Activity) view.getContext()).getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING);
        editing = true; imeSeen = false; editGen++; batch = 0; batchChanged = false;
        view.setFocusable(true);
        view.setFocusableInTouchMode(true);
        view.requestFocus();
        InputMethodManager imm = imm();
        if (imm != null) imm.restartInput(view);
        view.removeCallbacks(showIme);
        view.post(showIme);
        view.removeCallbacks(caret);
        view.postDelayed(caret, CARET_MS);
        view.invalidate();
        changed.run();
    }

    private void showIme() {
        InputMethodManager imm = imm();
        if (editing && imm != null) imm.showSoftInput(view, 0);
    }


    void end() {
        if (!editing) return;
        editing = false; imeSeen = false; editGen++; batch = 0;
        input.finishTyping();

        if (batchChanged) { batchChanged = false; searched.run(); }
        imeTop = Float.MAX_VALUE;
        view.removeCallbacks(showIme);
        view.removeCallbacks(caret);
        InputMethodManager imm = imm();
        if (imm != null) { imm.hideSoftInputFromWindow(view.getWindowToken(), 0); imm.restartInput(view); }
        view.setFocusableInTouchMode(false);
        view.clearFocus();
        view.invalidate();
        changed.run();
    }

    private void caretTick() { if (editing) { readIme(view.getRootWindowInsets()); view.invalidate(); view.postDelayed(caret, CARET_MS); } }



    private void readIme(WindowInsets in) {
        if (in == null) return;
        boolean visible = in.isVisible(WindowInsets.Type.ime());
        int bottom = in.getInsets(WindowInsets.Type.ime()).bottom;

        View root = view.getRootView(); boolean whole = GameRect.ui().whole();
        float winH = whole || root == null || root.getHeight() <= 0 ? view.getHeight() : root.getHeight();
        imeTop = visible && bottom > 0 ? winH - bottom - (whole ? 0 : view.getTop()) : Float.MAX_VALUE;
        if (!editing) return;
        if (visible) imeSeen = true;
        else if (imeSeen) view.post(this::end);
    }


    void insets(WindowInsets in) { readIme(in); view.invalidate(); }


    InputConnection connection(EditorInfo out) {
        if (!editing) return null;

        out.inputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD;
        out.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN | EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_ACTION_SEARCH;
        out.initialSelStart = out.initialSelEnd = input.length();
        out.setInitialSurroundingText(input.text());
        return new SearchConnection(editGen);
    }


    private void edited(boolean text) {
        if (text) {
            if (batch > 0) batchChanged = true;
            else searched.run();
        }
        InputMethodManager imm = imm();
        if (imm != null && editing) imm.updateSelection(view, input.length(), input.length(), input.composingStart(), input.composingEnd());
        view.invalidate();
    }



    private final class SearchConnection extends BaseInputConnection {
        private final int gen;
        SearchConnection(int gen) { super(view, false); this.gen = gen; }
        private boolean live() { return editing && gen == editGen; }

        @Override public boolean commitText(CharSequence text, int newCursorPosition) {
            if (!live()) return false;
            boolean enter = text != null && text.toString().indexOf('\n') >= 0;
            edited(input.commit(text));
            if (enter) view.post(SearchTyping.this::end);
            return true;
        }
        @Override public boolean setComposingText(CharSequence text, int newCursorPosition) { if (!live()) return false; edited(input.setComposing(text)); return true; }

        @Override public boolean setComposingRegion(int start, int end) { if (!live()) return false; boolean ok = input.markComposing(start, end); edited(false); return ok; }
        @Override public boolean finishComposingText() { if (live()) edited(input.finishComposing()); return true; }

        @Override public boolean deleteSurroundingText(int before, int after) { if (!live()) return false; edited(input.deleteUnits(before)); return true; }
        @Override public boolean deleteSurroundingTextInCodePoints(int before, int after) { if (!live()) return false; edited(input.deleteCodePoints(before)); return true; }
        @Override public CharSequence getTextBeforeCursor(int n, int flags) { String t = input.text(); return t.substring(Math.max(0, t.length() - Math.max(0, n))); }
        @Override public CharSequence getTextAfterCursor(int n, int flags) { return ""; }
        @Override public CharSequence getSelectedText(int flags) { return null; }
        @Override public SurroundingText getSurroundingText(int before, int after, int flags) {
            String t = input.text();
            int from = Math.max(0, t.length() - Math.max(0, before));
            return new SurroundingText(t.substring(from), t.length() - from, t.length() - from, from);
        }
        @Override public ExtractedText getExtractedText(ExtractedTextRequest request, int flags) { return null; }
        @Override public boolean setSelection(int start, int end) { return start == input.length() && end == input.length(); }
        @Override public int getCursorCapsMode(int reqModes) { return 0; }
        @Override public boolean commitCompletion(android.view.inputmethod.CompletionInfo text) { return false; }
        @Override public boolean commitCorrection(android.view.inputmethod.CorrectionInfo info) { return false; }
        @Override public boolean replaceText(int start, int end, CharSequence text, int newCursorPosition, TextAttribute attribute) {
            if (!live() || end != input.length() || start < 0 || start > end) return false;
            edited(input.replaceFrom(start, text));
            return true;
        }
        @Override public boolean beginBatchEdit() { if (live()) batch++; return live(); }
        @Override public boolean endBatchEdit() {
            if (!live()) return false;
            if (batch > 0 && --batch == 0 && batchChanged) { batchChanged = false; edited(true); }
            return batch > 0;
        }
        @Override public boolean performEditorAction(int action) { if (live()) view.post(SearchTyping.this::end); return true; }
        @Override public boolean sendKeyEvent(KeyEvent e) { if (live()) key(e); return true; }
    }



    boolean key(KeyEvent e) {
        if (!editing) return false;
        int code = e.getKeyCode();
        if (e.getAction() != KeyEvent.ACTION_DOWN) return code != KeyEvent.KEYCODE_BACK;
        switch (code) {
            case KeyEvent.KEYCODE_DEL: edited(input.backspace()); return true;
            case KeyEvent.KEYCODE_ENTER: case KeyEvent.KEYCODE_NUMPAD_ENTER: case KeyEvent.KEYCODE_SEARCH: end(); return true;
            default:
                int ch = e.getUnicodeChar();

                if ((ch & KeyCharacterMap.COMBINING_ACCENT) != 0) { edited(input.dead(ch & KeyCharacterMap.COMBINING_ACCENT_MASK)); return true; }
                if (ch > 0) { edited(input.type(ch, KeyCharacterMap::getDeadChar)); return true; }
                return false;
        }
    }


    boolean takesKey(KeyEvent e) {
        int code = e.getKeyCode();
        return editing && !PadInput.isPadKey(e) && code != KeyEvent.KEYCODE_BACK && code != KeyEvent.KEYCODE_ESCAPE;
    }



    boolean preIme(KeyEvent e) {
        if (!editing) return false;
        int code = e.getKeyCode();
        if (PadInput.isPadKey(e) && view.getContext() instanceof Activity) { ((Activity) view.getContext()).dispatchKeyEvent(e); return true; }
        if (code == KeyEvent.KEYCODE_BACK || code == KeyEvent.KEYCODE_ESCAPE) { if (e.getAction() == KeyEvent.ACTION_UP) end(); return true; }
        return false;
    }
}
