#!/usr/bin/env python3






import os
import re
import sys

DM = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
sys.path.insert(0, os.path.join(DM, 'ios', 'tools'))
POTIONS = {'hp1': 'lesser_healing_potion', 'hp2': 'light_healing_potion', 'hp3': 'healing_potion2', 'hp4': 'strong_healing_potion', 'hp5': 'greater_healing_potion',
           'mp1': 'lesser_mana_potion', 'mp2': 'light_mana_potion', 'mp3': 'mana_potion2', 'mp4': 'strong_mana_potion', 'mp5': 'greater_mana_potion',
           'rv1': 'rejuv_potion', 'rv2': 'full_rejuv_potion'}









FRAMES = {'menu_icon': ('PANEL/HUD_02/PauseMenu', 0), 'menu_icon_down': ('PANEL/HUD_02/PauseMenu', 1),
          'button': ('PANEL/Modals/ModalButton', 0), 'button_down': ('PANEL/Modals/ModalButton', 1),
          'build_inset': ('Controller/Panel/CharacterSheet/V2/CharacterSheet_Inset', 0),
          'build_modal': ('Controller/Panel/Modal/ui-console-modal', 0),
          'build_row': ('PANEL/Chronicle/Slot_list_XL', 0),
          'build_row_icon': ('PANEL/Chronicle/Slot_list_item_XL', 0),
          'build_long0': ('PANEL/Chronicle/Slot_list_long', 0), 'build_long1': ('PANEL/Chronicle/Slot_list_long', 1),
          'build_chip': ('PANEL/AdvancedCharacterSheet/AdvancedCharacterSheet_ValueBox', 0),
          'build_check0': ('Lobby/CreateGame/CreateGame_AdvancedCheckbox', 0), 'build_check1': ('Lobby/CreateGame/CreateGame_AdvancedCheckbox', 4),
          'build_divider': ('Controller/Panel/AdvancedCharacterSheet/AdvancedCharacterSheet_Divider', 0),
          'build_tree': ('Controller/Panel/Spells/V2/CenterPanelBG_SkillTrees', 0),
          'build_tab_on': ('Controller/Panel/Stash/V2/StashTabs', 0), 'build_tab_off': ('Controller/Panel/Stash/V2/StashTabs', 1)}


TILES = {'build_focus': 'Controller/Panel/ControllerSelector/ControllerSelectorBorder'}


TREES = {'ama': 'Am', 'sor': 'So', 'nec': 'Ne', 'pal': 'Pa', 'bar': 'Ba', 'dru': 'Dr', 'ass': 'As', 'war': 'Wa'}
FONTS = {'title.otf': 'exocetblizzardot-medium.otf', 'text.ttf': 'blizzardglobal-v5_81.ttf'}


DMG_COLOURS = (('physical', 'FontColorWhite'), ('fire', 'FontColorRed'), ('cold', 'FontColorBlue'), ('lightning', 'FontColorYellow'), ('poison', 'FontColorGreen'), ('magic', 'FontColorOrange'))


def dmg_colours(romfs):

    import romfs_art
    import profile_hd
    named = profile_hd.parse(romfs_art.read(romfs, profile_hd.FILES[profile_hd.HD]).decode('utf-8', 'replace'))
    out = []
    for key, token in DMG_COLOURS:
        c = profile_hd.colour(named.get(token), named)
        if c: out.append('%s %d %d %d' % (key, c[0], c[1], c[2]))
    return out










PAD_GLYPHS = {'a': ('ButtonRight', 'ButtonBottom'), 'b': ('ButtonBottom', 'ButtonRight'), 'start': ('ButtonStart', 'ButtonStart'),
              'left': ('DpadLeft', 'DpadLeft'), 'right': ('DpadRight', 'DpadRight'), 'y': ('ButtonLeft', 'ButtonTop')}


def frame_strip(romfs, path):


    import struct
    import romfs_art
    from PIL import Image
    b = romfs_art.read(romfs, path)
    fw, n = struct.unpack_from('<H', b, 6)[0], struct.unpack_from('<H', b, 20)[0] or 1
    im = romfs_art.sprite(romfs, path)
    pitch = im.width // n
    out = Image.new('RGBA', (fw * n, im.height))
    for i in range(n): out.paste(im.crop((i * pitch, 0, i * pitch + fw, im.height)), (i * fw, 0))
    return out


def stage_pad(romfs, out):


    import romfs_art
    d = os.path.join(out, 'pad')
    os.makedirs(d, exist_ok=True)
    frame_strip(romfs, 'hd/global/ui/Controller/Cursor/Hold.sprite').save(os.path.join(d, 'hold.png'))
    romfs_art.sprite(romfs, 'hd/global/ui/Controller/HoverImages/ModalButton_Hover.sprite').save(os.path.join(d, 'focus.png'))
    romfs_art.sprite(romfs, 'hd/global/ui/Controller/Panel/MainPanelBG_FooterBar.sprite').save(os.path.join(d, 'footer.png'))
    for style in ('switch', 'xbox', 'ps5'):
        os.makedirs(os.path.join(d, style), exist_ok=True)
        for name, src in PAD_GLYPHS.items():
            romfs_art.frame(romfs, 'hd/global/ui/Controller/ControllerButtons/%s/%s.sprite' % (style, src[0 if style == 'switch' else 1]), 0).save(os.path.join(d, style, name + '.png'))




ROLE_FONTS = ('ExocetBlizzardOT-Medium.otf', 'Formal436BT.ttf', 'Philosopher-BoldItalic.ttf', 'KoDia.ttf', 'IrisL.ttf', 'BlizzardGlobal-V5_81.ttf')







QUICK_PANEL = ('Controller/Panel/EscapeMenu/EscapeOptionsBG', (226, 78, 670, 630))
QUICK_SPRITES = {'focus': ('Controller/HoverImages/EscapeOptionsButton_Hover', 0), 'divider': ('PauseMenu/Divider_Medium', 0), 'modal_bg': ('PANEL/Modals/Modal_BG', 0)}
QUICK_STRIPS = {'button': 'Controller/Panel/EscapeMenu/EscapeOptions_Button', 'arrow_l': 'PauseMenu/LeftArrow', 'arrow_r': 'PauseMenu/RightArrow'}







PANEL_SPRITES = {'frame': ('Controller/Panel/MainPanelBG', 0), 'endcap_l': ('Controller/Panel/Top_Tabs_EndCap_Left', 0), 'endcap_r': ('Controller/Panel/Top_Tabs_EndCap_Right', 0)}
PANEL_SPRITES.update({'tab%d_%d' % (n, f): ('Controller/Panel/Controller_Top_Tab_%dTabs' % n, f) for n in (5, 6, 7) for f in (0, 1)})
PANEL_SPRITES.update({'tabfocus%d' % n: ('Controller/HoverImages/ControllerTopTabs_%d' % n, 0) for n in (5, 6, 7)})
PANEL_SPRITES.update({'foot_%d' % f: ('FrontEnd/HD/Final/FrontEnd_ButtonMed', f) for f in range(4)})
PANEL_CLOSE = os.path.join(DM, 'ios', 'NativeHUD', 'touch', 'close.png')
PANEL_FILES = tuple('panel/' + n for n in PANEL_SPRITES) + ('panel/close',)
QUICK_FILES = ('quick/panel',) + tuple('quick/%s_%d' % (n, i) for n in QUICK_STRIPS for i in range(4)) + tuple('quick/' + n for n in QUICK_SPRITES)



UI_COLOURS = ('FontColorLightGold', 'FontColorGoldYellow', 'FontColorWhite', 'FontColorGray', 'FontColorLightGray', 'FontColorDarkGrayGold')




VERIFY_FILES = (tuple(f + '.png' for f in QUICK_FILES) + ('pad/footer.png', 'pad/focus.png', 'pad/hold.png')
                + tuple('pad/%s/%s.png' % (st, g) for st in ('switch', 'xbox', 'ps5') for g in ('a', 'b', 'start', 'left', 'right', 'y'))
                + ('button.png', 'button_down.png', 'build_modal.png') + tuple(f + '.png' for f in PANEL_FILES) + tuple('fonts/' + f for f in ROLE_FONTS) + ('ui_colors.txt',))


def profile_colour(text, token, depth=0):


    m = re.search(r'"%s"\s*:\s*\{\s*"r"\s*:\s*(\d+)\s*,\s*"g"\s*:\s*(\d+)\s*,\s*"b"\s*:\s*(\d+)' % re.escape(token), text)
    if m: return tuple(int(v) for v in m.groups()) if all(int(v) < 256 for v in m.groups()) else None
    a = re.search(r'"%s"\s*:\s*"\$(\w+)"' % re.escape(token), text)
    return profile_colour(text, a.group(1), depth + 1) if a and depth < 8 else None


def ui_colours(romfs):
    import romfs_art
    text = romfs_art.read(romfs, 'global/ui/layouts/_profilehd.json').decode('utf-8', 'replace')
    out = []
    for token in UI_COLOURS:
        c = profile_colour(text, token)
        if c is None: raise SystemExit('appart: game text colour %s not found in _profilehd.json' % token)
        out.append('%s %d %d %d' % ((token,) + c))
    return out


def stage_quick(romfs, out):


    import romfs_art
    d = os.path.join(out, 'quick')
    os.makedirs(d, exist_ok=True)
    path, box = QUICK_PANEL
    romfs_art.sprite(romfs, 'hd/global/ui/%s.sprite' % path).crop(box).save(os.path.join(d, 'panel.png'))
    for name, (path, i) in QUICK_SPRITES.items(): romfs_art.frame(romfs, 'hd/global/ui/%s.sprite' % path, i).save(os.path.join(d, name + '.png'))
    for name, path in QUICK_STRIPS.items():
        for i in range(4): romfs_art.frame(romfs, 'hd/global/ui/%s.sprite' % path, i).save(os.path.join(d, '%s_%d.png' % (name, i)))
    with open(os.path.join(out, 'ui_colors.txt'), 'w') as f: f.write(''.join(l + '\n' for l in ui_colours(romfs)))


def pitch_frame(romfs, path, i):



    import struct
    import romfs_art
    b = romfs_art.read(romfs, path)
    fw, n = struct.unpack_from('<H', b, 6)[0], struct.unpack_from('<H', b, 20)[0] or 1
    im = romfs_art.sprite(romfs, path)
    pitch = im.width // n
    if i >= n: raise SystemExit('appart: %s has %d frames, frame %d asked' % (path, n, i))
    return im.crop((i * pitch, 0, i * pitch + fw, im.height))


def stage_panel(romfs, out):


    import shutil
    import romfs_art
    d = os.path.join(out, 'panel')
    os.makedirs(d, exist_ok=True)
    for name, (path, i) in PANEL_SPRITES.items(): pitch_frame(romfs, 'hd/global/ui/%s.sprite' % path, i).save(os.path.join(d, name + '.png'))
    shutil.copyfile(PANEL_CLOSE, os.path.join(d, 'close.png'))


def missing(out):

    return [f for f in VERIFY_FILES if not os.path.isfile(os.path.join(out, f)) or os.path.getsize(os.path.join(out, f)) == 0]


def stage(romfs, out):
    import struct
    import romfs_art
    os.makedirs(out, exist_ok=True)
    import gen_exp_rail; gen_exp_rail.stage(romfs, os.path.join(out, 'exp'))
    import gen_inv_pics; gen_inv_pics.stage(romfs, out)
    for name, asset in POTIONS.items():
        romfs_art.sprite(romfs, 'hd/global/ui/items/misc/potion/%s.sprite' % asset).save(os.path.join(out, name + '.png'))
    for name, (path, i) in FRAMES.items():
        romfs_art.frame(romfs, 'hd/global/ui/%s.sprite' % path, i).save(os.path.join(out, name + '.png'))
    for name, path in TILES.items():
        p = 'hd/global/ui/%s.sprite' % path
        fw = struct.unpack_from('<H', romfs_art.read(romfs, p), 6)[0]; im = romfs_art.sprite(romfs, p)
        for i in range(im.width // fw): im.crop((i * fw, 0, (i + 1) * fw, im.height)).save(os.path.join(out, '%s_%d.png' % (name, i)))
    for code, short in TREES.items():
        for i in range(3): romfs_art.frame(romfs, 'hd/global/ui/Controller/Panel/Spells/V2/%sSkillTree.sprite' % short, i).save(os.path.join(out, 'build_tree_%s_%d.png' % (code, i)))
    romfs_art.sprite(romfs, 'hd/global/ui/Controller/Panel/Options/Panel_Options_BG.sprite').save(os.path.join(out, 'panel.png'))
    for name, font in FONTS.items():
        with open(os.path.join(out, name), 'wb') as f: f.write(romfs_art.read(romfs, 'hd/ui/fonts/' + font))
    os.makedirs(os.path.join(out, 'fonts'), exist_ok=True)
    for font in ROLE_FONTS:
        with open(os.path.join(out, 'fonts', font), 'wb') as f: f.write(romfs_art.read(romfs, 'hd/ui/fonts/' + font))
    try: lines = dmg_colours(romfs)
    except (Exception, SystemExit) as e: lines = []; print('appart: damage number colours not read (%s); the App uses its built-in ones' % e)
    if len(lines) < len(DMG_COLOURS): print('appart: only %d of %d damage number colours found' % (len(lines), len(DMG_COLOURS)))
    with open(os.path.join(out, 'dmg_colors.txt'), 'w') as f: f.write(''.join(l + '\n' for l in lines))


    stage_pad(romfs, out)
    stage_quick(romfs, out)
    stage_panel(romfs, out)


if __name__ == '__main__':
    stage(sys.argv[1], sys.argv[2])
