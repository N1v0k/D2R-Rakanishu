
package android.app;

public class ApplicationExitInfo {
    private final int pid, reason, status;

    public ApplicationExitInfo(int pid, int reason, int status) { this.pid = pid; this.reason = reason; this.status = status; }
    public int getPid() { return pid; }
    public int getReason() { return reason; }
    public int getStatus() { return status; }
}
