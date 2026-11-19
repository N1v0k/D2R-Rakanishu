

package android.content.pm;

import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Icon;

public class ShortcutInfo {
    private String id = "";

    public String getId() { return id; }

    public static final class Builder {
        private final ShortcutInfo info = new ShortcutInfo();
        public Builder(Context c, String id) { info.id = id; }
        public Builder setShortLabel(CharSequence s) { return this; }
        public Builder setLongLabel(CharSequence s) { return this; }
        public Builder setIcon(Icon i) { return this; }
        public Builder setIntent(Intent i) { return this; }
        public ShortcutInfo build() { return info; }
    }
}
