


#ifndef DM_SPLASH_RULE_H
#define DM_SPLASH_RULE_H

typedef enum { DM_SPLASH_NONE, DM_SPLASH_HEAVY, DM_SPLASH_MEDIUM, DM_SPLASH_LIGHT, DM_SPLASH_THRUST, DM_SPLASH_UNKNOWN } dm_splash_group;




dm_splash_group dm_splash_group_of(const char *type, int two_hands);

typedef struct {
    int share;
    int width;
    int depth;
} dm_splash_area;





dm_splash_area dm_splash_area_of(dm_splash_group g, int cls, int str, int dex, int range);






double dm_splash_cone_factor(const dm_splash_area *a, int ax, int ay, int px, int py, int nx, int ny);




double dm_splash_reach(const dm_splash_area *a, int extra);



int dm_splash_skill_excluded(int kick, int srvdofunc);

#endif
