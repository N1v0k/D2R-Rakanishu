package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;




final class BookJson {
    private final String s;
    private int i;

    private BookJson(String s) { this.s = s; }

    static Object parse(String text) {
        BookJson p = new BookJson(text);
        p.space();
        Object v = p.value();
        p.space();
        if (p.i != text.length()) throw p.fail("text after the value");
        return v;
    }

    private IllegalArgumentException fail(String what) { return new IllegalArgumentException("book.json: " + what + " at " + i); }

    private void space() { while (i < s.length() && (s.charAt(i) == ' ' || s.charAt(i) == '\n' || s.charAt(i) == '\r' || s.charAt(i) == '\t')) i++; }

    private Object value() {
        if (i >= s.length()) throw fail("end of text");
        char c = s.charAt(i);
        if (c == '{') return object();
        if (c == '[') return array();
        if (c == '"') return string();
        if (s.startsWith("true", i)) { i += 4; return Boolean.TRUE; }
        if (s.startsWith("false", i)) { i += 5; return Boolean.FALSE; }
        if (s.startsWith("null", i)) { i += 4; return null; }
        return number();
    }

    private Map<String, Object> object() {
        Map<String, Object> m = new HashMap<>();
        i++;
        space();
        if (i < s.length() && s.charAt(i) == '}') { i++; return m; }
        while (true) {
            space();
            if (i >= s.length() || s.charAt(i) != '"') throw fail("a key");
            String k = string();
            space();
            if (i >= s.length() || s.charAt(i) != ':') throw fail("':'");
            i++;
            space();
            m.put(k, value());
            space();
            if (i >= s.length()) throw fail("end of text in an object");
            char c = s.charAt(i++);
            if (c == '}') return m;
            if (c != ',') throw fail("',' or '}'");
        }
    }

    private List<Object> array() {
        List<Object> a = new ArrayList<>();
        i++;
        space();
        if (i < s.length() && s.charAt(i) == ']') { i++; return a; }
        while (true) {
            space();
            a.add(value());
            space();
            if (i >= s.length()) throw fail("end of text in an array");
            char c = s.charAt(i++);
            if (c == ']') return a;
            if (c != ',') throw fail("',' or ']'");
        }
    }

    private String string() {
        StringBuilder b = new StringBuilder();
        i++;
        while (true) {
            if (i >= s.length()) throw fail("end of text in a string");
            char c = s.charAt(i++);
            if (c == '"') return b.toString();
            if (c != '\\') { b.append(c); continue; }
            if (i >= s.length()) throw fail("end of text in an escape");
            char e = s.charAt(i++);
            switch (e) {
                case '"': case '\\': case '/': b.append(e); break;
                case 'b': b.append('\b'); break;
                case 'f': b.append('\f'); break;
                case 'n': b.append('\n'); break;
                case 'r': b.append('\r'); break;
                case 't': b.append('\t'); break;
                case 'u':
                    if (i + 4 > s.length()) throw fail("a short \\u escape");
                    try { b.append((char) Integer.parseInt(s.substring(i, i + 4), 16)); } catch (NumberFormatException x) { throw fail("a bad \\u escape"); }
                    i += 4;
                    break;
                default: throw fail("the escape \\" + e);
            }
        }
    }

    private Object number() {
        int start = i;
        if (i < s.length() && s.charAt(i) == '-') i++;
        boolean whole = true;
        while (i < s.length()) {
            char c = s.charAt(i);
            if (c >= '0' && c <= '9') i++;
            else if (c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') { whole = false; i++; }
            else break;
        }
        String t = s.substring(start, i);
        if (t.isEmpty() || t.equals("-")) throw fail("a value");
        try { return whole ? (Object) Long.parseLong(t) : (Object) Double.parseDouble(t); } catch (NumberFormatException x) { throw fail("the number " + t); }
    }
}
