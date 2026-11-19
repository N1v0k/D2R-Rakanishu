

package android.content;

public class Intent {
    public static final String ACTION_MAIN = "android.intent.action.MAIN";

    public Intent(Context c, Class<?> cls) { }
    public Intent setAction(String a) { return this; }
    public Intent putExtra(String key, boolean value) { return this; }
}
