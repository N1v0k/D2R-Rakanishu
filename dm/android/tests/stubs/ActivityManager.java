

package android.app;

import java.util.ArrayList;
import java.util.List;

public class ActivityManager {
    public final List<ApplicationExitInfo> records = new ArrayList<>();

    public List<ApplicationExitInfo> getHistoricalProcessExitReasons(String pkg, int pid, int max) {
        List<ApplicationExitInfo> out = new ArrayList<>();
        for (ApplicationExitInfo e : records) if ((pid == 0 || e.getPid() == pid) && (max == 0 || out.size() < max)) out.add(e);
        return out;
    }
}
