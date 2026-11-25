









#define DM_MODE_ITEMS(X) \
    X("符文之语清单", 2, dm_mode_none, dm_runeword_ui_rebuild) \
    X("前端路径理顺与替按键", 0, dm_mode_none, dm_ui_auto_mode_leave) \
    X("界面触摸缓存与挪位表", 2, dm_mode_none, dm_ui_cache_mode_leave) \
    X("主菜单构建号与新版本提示", 2, dm_update_mode_enter, dm_mode_none) \
    X("死亡弹窗按当前模式重开", 2, dm_dead_modal_mode_enter, dm_mode_none) \
    X("魔盒并排", 0, dm_mode_none, dm_cube_side_mode_leave)
