





#include <Python.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static int fail(PyStatus st) {
    if (PyStatus_IsExit(st)) return st.exitcode;
    fprintf(stderr, "launcher: %s%s%s\n", st.func ? st.func : "", st.func ? ": " : "", st.err_msg ? st.err_msg : "error");
    return 120;
}

static PyStatus add_path(PyConfig *c, const char *home, const char *sub) {
    char buf[PATH_MAX];
    if (snprintf(buf, sizeof buf, "%s/%s", home, sub) >= (int)sizeof buf) return PyStatus_Error("runtime path too long");
    wchar_t *w = Py_DecodeLocale(buf, NULL);
    if (!w) return PyStatus_NoMemory();
    PyStatus st = PyWideStringList_Append(&c->module_search_paths, w);
    PyMem_RawFree(w);
    return st;
}

int main(int argc, char **argv) {
    const char *home = getenv("DM_PY_HOME");
    if (!home || !*home) { fprintf(stderr, "launcher: DM_PY_HOME is not set\n"); return 120; }
    char self[PATH_MAX];
    if (!realpath("/proc/self/exe", self)) { perror("launcher: /proc/self/exe"); return 120; }

    PyPreConfig pre;
    PyPreConfig_InitIsolatedConfig(&pre);
    pre.utf8_mode = 1;
    PyStatus st = Py_PreInitialize(&pre);
    if (PyStatus_Exception(st)) return fail(st);

    PyConfig c;
    PyConfig_InitIsolatedConfig(&c);
    c.isolated = 0;
    c.use_environment = 0;
    c.user_site_directory = 0;
    c.safe_path = 0;
    c.parse_argv = 1;
    c.module_search_paths_set = 1;
    st = PyConfig_SetBytesString(&c, &c.home, home);
    if (!PyStatus_Exception(st)) st = PyConfig_SetBytesString(&c, &c.executable, self);
    if (!PyStatus_Exception(st)) st = PyConfig_SetBytesString(&c, &c.program_name, self);
    if (!PyStatus_Exception(st)) st = add_path(&c, home, "lib/python3.14");
    if (!PyStatus_Exception(st)) st = add_path(&c, home, "lib/python3.14/lib-dynload");
    if (!PyStatus_Exception(st)) st = add_path(&c, home, "site-packages");
    if (!PyStatus_Exception(st)) st = PyConfig_SetBytesArgv(&c, argc, argv);
    if (!PyStatus_Exception(st)) st = Py_InitializeFromConfig(&c);
    PyConfig_Clear(&c);
    if (PyStatus_Exception(st)) return fail(st);
    return Py_RunMain();
}
