package com.xiaoboluo.d2r;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;


public class InvPicturesTest {
    static int failed;
    static void check(String what, Object got, Object want) {
        if (String.valueOf(got).equals(String.valueOf(want))) return;
        failed++; System.out.println("FAIL " + what + ": got " + got + ", want " + want);
    }

    static InvPictures pics(int variants) {
        Map<String, String> base = new HashMap<>(), unique = new HashMap<>(), set = new HashMap<>();
        Map<String, Integer> v = new HashMap<>();
        base.put("b1", "pic_b"); base.put("b2", "pic_c");
        unique.put("7", "own_u"); set.put("3", "own_s");
        v.put("pic_b", variants);
        return new InvPictures(base, unique, set, v);
    }

    public static void main(String[] args) {
        InvPictures p = pics(3);
        check("unique by row", p.name("b1", 7, 7, 2), "own_u");
        check("set by row", p.name("b1", 5, 3, -1), "own_s");
        check("unique without own picture: base variant", p.name("b1", 7, 8, 0), "pic_b1");
        check("magic: variant by picture index", p.name("b1", 4, -1, 2), "pic_b3");
        check("index past the variants: base", p.name("b1", 4, -1, 3), "pic_b");
        check("no index: base", p.name("b1", 4, -1, -1), "pic_b");
        check("base without variants", p.name("b2", 4, -1, 1), "pic_c");
        check("unknown code", p.name("zz", 4, -1, 0), "null");

        Set<String> have = new HashSet<>();
        for (String n : new String[]{"pic_b", "pic_b1", "pic_b2", "pic_b3", "pic_c", "own_u", "own_s"}) have.add(n);
        check("complete", p.missing(have::contains), "null");
        have.remove("pic_b2");
        check("a missing variant", p.missing(have::contains), "inv/pic_b2.png");
        have.add("pic_b2"); have.remove("own_s");
        check("a missing own picture", p.missing(have::contains), "inv/own_s.png");
        check("variant count below 1", pics(0).missing(n -> true), "inv.json");
        check("no bases", new InvPictures(new HashMap<>(), new HashMap<>(), new HashMap<>(), new HashMap<>()).missing(n -> true), "inv.json");

        System.out.println(failed == 0 ? "InvPicturesTest: all ok" : failed + " FAILED");
        System.exit(failed == 0 ? 0 : 1);
    }
}
