


package android.content;

import android.app.ActivityManager;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import java.io.File;

public class Context {
    public File filesDir, externalDir;
    public ApplicationInfo appInfo = new ApplicationInfo();
    public PackageManager packageManager = new PackageManager();
    public ActivityManager activityManager;

    public File getFilesDir() { return filesDir; }
    public File getExternalFilesDir(String type) { return externalDir; }
    public ApplicationInfo getApplicationInfo() { return appInfo; }
    public PackageManager getPackageManager() { return packageManager; }
    public String getPackageName() { return "com.xiaoboluo.d2r"; }
    public String getString(int id) { return "string/" + id; }
    public <T> T getSystemService(Class<T> cls) { return cls == ActivityManager.class ? cls.cast(activityManager) : null; }
}
