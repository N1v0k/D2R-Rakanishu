package com.xiaoboluo.d2r;

import java.io.File;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;



public final class PlayerHooksTest {
    private static int failures;

    private static void check(boolean ok, String what) {
        if (!ok) { failures++; System.out.println("FAIL " + what); }
    }

    private static Object sample(Class<?> t) {
        if (t == File.class) return new File("standard");
        if (t == List.class) return new ArrayList<>(List.of("standard"));
        if (t == long.class) return 7L;
        if (t == int.class) return 7;
        if (t == float.class) return 7f;
        if (t == String.class) return "standard";
        if (t == PackPlan.class) return new PackPlan(1, 1, 1, PackPlan.SERIAL);
        return null;
    }

    public static void main(String[] args) throws Exception {
        int n = 0;
        for (Method m : TestHooks.class.getDeclaredMethods()) {
            if (!Modifier.isStatic(m.getModifiers()) || m.isSynthetic()) continue;
            n++;
            m.setAccessible(true);
            Class<?>[] types = m.getParameterTypes();
            Object[] in = new Object[types.length];
            for (int i = 0; i < types.length; i++) in[i] = sample(types[i]);
            Object out = m.invoke(null, in);
            Class<?> r = m.getReturnType();
            Object handed = null;
            for (int i = 0; i < types.length; i++) if (types[i] == r && in[i] != null) handed = in[i];
            String name = m.getName();
            if (r == void.class) continue;
            if (r == boolean.class) check(Boolean.FALSE.equals(out), name + " returns false");
            else if (handed != null) check(handed.equals(out) && (r.isPrimitive() || handed == out), name + " returns what it was handed");
            else check(out == null, name + " returns null");
        }
        check(n >= 20, "player TestHooks has its hooks (" + n + ")");
        check(TestHooks.packJob(null) == null, "packJob gives no build job");
        Path root = Path.of(System.getProperty("dm.root"));
        String activity = Files.readString(root.resolve("dm/android/app/src/main/java/com/xiaoboluo/d2r/DmActivity.java"));
        check(activity.contains("static final boolean PACK_BUILD_OFFERED = BuildConfig.DM_PACK_BUILDER;"), "PACK_BUILD_OFFERED follows BuildConfig.DM_PACK_BUILDER");
        String gradle = Files.readString(root.resolve("dm/android/app/build.gradle"));
        check(gradle.contains("buildConfigField 'boolean', 'DM_PACK_BUILDER', pyRuntime && shaderXlate ? 'true' : 'false'"),
              "DM_PACK_BUILDER is the runtime and the translator together");
        if (failures > 0) throw new AssertionError(failures + " failures");
        System.out.println("PlayerHooksTest: OK (" + n + " hooks)");
    }
}
