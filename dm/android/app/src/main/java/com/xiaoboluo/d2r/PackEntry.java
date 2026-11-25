package com.xiaoboluo.d2r;

import java.util.List;
import java.util.Map;











final class PackEntry {
    enum Head { INTRO, UPDATED, VERSION, NONE }
    enum Target { PICK, KEPT }
    enum Kept { NONE, PARTIAL, COMPLETE }

    final Head head;
    final String version;
    final boolean rebuild;
    final Target target;
    final boolean missing;

    private PackEntry(Head head, String version, boolean rebuild, Target target, boolean missing) {
        this.head = head; this.version = version; this.rebuild = rebuild; this.target = target; this.missing = missing;
    }



    static PackEntry decide(String have, PackSource.Record record, String product, String version, Kept kept) {
        boolean phone = record != null && record.source == PackSource.Kind.PHONE, complete = kept == Kept.COMPLETE;
        Target target = complete ? Target.KEPT : Target.PICK;
        boolean missing = target == Target.PICK && (kept == Kept.PARTIAL || phone && have != null);
        if (have == null) return new PackEntry(Head.INTRO, null, false, Target.PICK, false);
        if (PackSource.matches(have, product)) return new PackEntry(Head.NONE, null, phone || complete, target, missing);
        if (phone) return new PackEntry(Head.UPDATED, null, true, target, missing);
        String other = version != null ? version : PackSource.product(have);
        return new PackEntry(other == null ? Head.NONE : Head.VERSION, other, false, target, missing);
    }


    interface Held { boolean held(PackJournal.Unit u); }
    static Kept kept(Map<String, PackJournal.Record> records, List<PackJournal.Unit> units, Held held) {
        int good = 0, seen = 0;
        for (PackJournal.Unit u : units) {
            PackJournal.Record r = records.get(u.id);
            if (r == null) continue;
            seen++;
            if (r.fingerprint.equals(u.fingerprint) && r.digest.equals(u.digest()) && held.held(u)) good++;
        }
        return good == units.size() && !units.isEmpty() ? Kept.COMPLETE : seen == 0 ? Kept.NONE : Kept.PARTIAL;
    }

    static boolean present(PackJournal.Unit u) {
        for (PackJournal.Output o : u.outputs) if (!o.file.isFile() || o.file.length() != o.size) return false;
        return true;
    }
}
