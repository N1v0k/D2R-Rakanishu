#!/usr/bin/env python3






import hashlib, json, os, re
import touch_layout

U = ''


TABLE = {
    'ui-controller.json': {
        'UseItemController': ('4c4adb8b', '双击使用'),
        'OpenHoradricCubeController': ('e2d89ec6', '双击打开'),
        'ReadScrollController': ('61e448cc', '双击阅读'),
        'UsePotionOfLifeController': ('2ccab0ae', '双击永久增加20点生命值'),
        'UseBookOfSkillController': ('619bdd63', '双击增加你的技能点'),
        'UseScrollOfResistanceController': ('bc185cbc', '双击施法'),
        'UseTokenOfAbsolutionController': ('9a7778b6', '双击重设属性技能点'),
        'UseTerrorTokenAct1Controller': ('aefa10c5', '双击恐惧化第一幕'),
        'UseTerrorTokenAct2Controller': ('3005cb15', '双击恐惧化第二幕'),
        'UseTerrorTokenAct3Controller': ('45921fbf', '双击恐惧化第三幕'),
        'UseTerrorTokenAct4Controller': ('5ebcc635', '双击恐惧化第四幕'),
        'UseTerrorTokenAct5Controller': ('0bd3c1fd', '双击恐惧化第五幕'),
        'comparisonTooltipController': ('0abebcab', ''),
        'hireComparisonTooltipController': ('2f147b1d', ''),
        'StrYouDiedConsole': ('5336efaf', '你挂了'),
        'ControllerInteractButton': ('54234e14', ''),
        'ControllerTitleScreenPrompt': ('d701b1f2', '点击屏幕开始'),
        'EnterCharacterName': ('c516bdc5', ''),


        'ControllerCycleComparisonTooltips': ('d53c0a25', '再点一次与另一件比较'),
        'SelectButtonNoteText': ('aa4e91fa', ''),
        'MoviePlaybackHoldToSkip_switch': ('f3a05873', ''),
    },
    'ui.json': {


        'LevelUpStatsNotificationController': ('7854f598', '你升级了。'),
        'LevelUpNotificationController': ('7ba311c9', '你升级了。'),
        'QuestLogNotificationController': ('7b65d87d', '任务已更新。'),
        'MoviePlaybackHoldToSkip': ('f3a05873', ''),


        'strchratr': ('703341e1', '%s命中值'),
        'strchrrat': ('703341e1', '%s命中值'),
    },
}





EN = {
    'ui-controller.json': {
        'UseItemController': ('3c07d84d', 'Double-tap to Use'),
        'OpenHoradricCubeController': ('3c06b741', 'Double-tap to Open'),
        'ReadScrollController': ('5b76ecf7', 'Double-tap to Read'),
        'UsePotionOfLifeController': ('74604765', 'Double-tap to permanently add 20 to Life'),
        'UseBookOfSkillController': ('3a031cc6', 'Double-tap to learn skill of your choice'),
        'UseScrollOfResistanceController': ('ce192aed', 'Double-tap to Cast'),
        'UseTokenOfAbsolutionController': ('3ba19ef3', 'Double-tap to reset Stat/Skill Points'),
        'UseTerrorTokenAct1Controller': ('73cb07eb', 'Double-tap to terrorize Act 1'),
        'UseTerrorTokenAct2Controller': ('8c394882', 'Double-tap to terrorize Act 2'),
        'UseTerrorTokenAct3Controller': ('558bcf92', 'Double-tap to terrorize Act 3'),
        'UseTerrorTokenAct4Controller': ('07eadb68', 'Double-tap to terrorize Act 4'),
        'UseTerrorTokenAct5Controller': ('9f1ff50e', 'Double-tap to terrorize Act 5'),
        'comparisonTooltipController': ('48cc0c36', ''),
        'hireComparisonTooltipController': ('a97423d0', ''),
        'StrYouDiedConsole': ('e2908c71', 'You have died'),
        'ControllerInteractButton': ('54234e14', ''),
        'ControllerTitleScreenPrompt': ('75c1a3c8', 'Tap the Screen to Begin'),
        'EnterCharacterName': ('8334a399', ''),
        'ControllerCycleComparisonTooltips': ('389c3ec9', 'Tap again to compare with the other'),
        'SelectButtonNoteText': ('e0bafb15', ''),
        'MoviePlaybackHoldToSkip_switch': ('dab56a9e', ''),
    },
    'ui.json': {
        'LevelUpStatsNotificationController': ('525fb31a', "You've leveled up."),
        'LevelUpNotificationController': ('1e4882b2', "You've leveled up."),
        'QuestLogNotificationController': ('94d110a8', 'Quest has been updated.'),
        'MoviePlaybackHoldToSkip': ('dab56a9e', ''),
    },
}



ES = {
    'ui-controller.json': {
        'UseItemController': ('b3c337b6', 'Toca dos veces para usar'),
        'OpenHoradricCubeController': ('316e4f76', 'Toca dos veces para abrir'),
        'ReadScrollController': ('9f9ebb2f', 'Toca dos veces para leer'),
        'UsePotionOfLifeController': ('8261013a', 'Toca dos veces para conseguir 20 p. de vida permanentes'),
        'UseBookOfSkillController': ('9b9ad5b4', 'Toca dos veces para aprender una habilidad'),
        'UseScrollOfResistanceController': ('2347c7a9', 'Toca dos veces para lanzar'),
        'UseTokenOfAbsolutionController': ('cce00a79', 'Toca dos veces para reiniciar los puntos de estadísticas/habilidades'),
        'UseTerrorTokenAct1Controller': ('9c7bb697', 'Toca dos veces para aterrorizar el Acto I'),
        'UseTerrorTokenAct2Controller': ('f46a3882', 'Toca dos veces para aterrorizar el Acto II'),
        'UseTerrorTokenAct3Controller': ('773c665e', 'Toca dos veces para aterrorizar el Acto III'),
        'UseTerrorTokenAct4Controller': ('19ba4849', 'Toca dos veces para aterrorizar el Acto IV'),
        'UseTerrorTokenAct5Controller': ('7dffa819', 'Toca dos veces para aterrorizar el Acto V'),
        'comparisonTooltipController': ('c84875e2', ''),
        'hireComparisonTooltipController': ('c84875e2', ''),
        'StrYouDiedConsole': ('c2b02f82', 'Has muerto'),
        'ControllerInteractButton': ('54234e14', ''),
        'ControllerTitleScreenPrompt': ('885ef015', 'Toca la pantalla para empezar'),
        'EnterCharacterName': ('be6e6664', ''),
        'ControllerCycleComparisonTooltips': ('1b3ed98b', 'Toca de nuevo para comparar con el otro'),
        'SelectButtonNoteText': ('c037151e', ''),
        'MoviePlaybackHoldToSkip_switch': ('a6b6a532', ''),
    },
    'ui.json': {
        'LevelUpStatsNotificationController': ('cbd9f839', 'Has subido de nivel.'),
        'LevelUpNotificationController': ('4b4b08fe', 'Has subido de nivel.'),
        'QuestLogNotificationController': ('13cf1af4', 'Se ha actualizado la búsqueda.'),
        'MoviePlaybackHoldToSkip': ('a6b6a532', ''),
    },
}

MX = {
    'ui-controller.json': {
        'UseItemController': ('b3c337b6', 'Toca dos veces para usar'),
        'OpenHoradricCubeController': ('316e4f76', 'Toca dos veces para abrir'),
        'ReadScrollController': ('9f9ebb2f', 'Toca dos veces para leer'),
        'UsePotionOfLifeController': ('c7d42535', 'Toca dos veces para agregar 20 a tu vida permanentemente'),
        'UseBookOfSkillController': ('906ec564', 'Toca dos veces para aprender una habilidad a elección'),
        'UseScrollOfResistanceController': ('76a87c82', 'Toca dos veces para activar'),
        'UseTokenOfAbsolutionController': ('5f21cbbb', 'Toca dos veces para restablecer los puntos de atributo/habilidad'),
        'UseTerrorTokenAct1Controller': ('37fbf40e', 'Toca dos veces para aterrorizar el Acto 1'),
        'UseTerrorTokenAct2Controller': ('a23ab72a', 'Toca dos veces para aterrorizar el Acto 2'),
        'UseTerrorTokenAct3Controller': ('65e32c9e', 'Toca dos veces para aterrorizar el Acto 3'),
        'UseTerrorTokenAct4Controller': ('0217c37c', 'Toca dos veces para aterrorizar el Acto 4'),
        'UseTerrorTokenAct5Controller': ('0478c1a6', 'Toca dos veces para aterrorizar el Acto 5'),
        'comparisonTooltipController': ('c84875e2', ''),
        'hireComparisonTooltipController': ('c84875e2', ''),
        'StrYouDiedConsole': ('0cd61240', 'Has muerto'),
        'ControllerInteractButton': ('54234e14', ''),
        'ControllerTitleScreenPrompt': ('3db5d4d5', 'Toca la pantalla para comenzar'),
        'EnterCharacterName': ('53243134', ''),
        'ControllerCycleComparisonTooltips': ('43363ba3', 'Toca de nuevo para comparar con el otro'),
        'SelectButtonNoteText': ('483ba8e7', ''),
        'MoviePlaybackHoldToSkip_switch': ('a6b6a532', ''),
    },
    'ui.json': {
        'LevelUpStatsNotificationController': ('c6faf1bb', 'Subiste de nivel.'),
        'LevelUpNotificationController': ('582b5596', 'Subiste de nivel.'),
        'QuestLogNotificationController': ('b9ebf5ae', 'Se actualizó la gesta.'),
        'MoviePlaybackHoldToSkip': ('a6b6a532', ''),
    },
}


RU = {
    'ui-controller.json': {
        'UseItemController': ('7474c613', 'Двойной тап — использовать'),
        'OpenHoradricCubeController': ('b702fc29', 'Двойной тап — открыть'),
        'ReadScrollController': ('1aca2594', 'Двойной тап — прочитать'),
        'UsePotionOfLifeController': ('1fc240c4', 'Двойной тап — +20 к запасу здоровья'),
        'UseBookOfSkillController': ('3230fdd2', 'Двойной тап — выучить умение'),
        'UseScrollOfResistanceController': ('a21bc33d', 'Двойной тап — применить'),
        'UseTokenOfAbsolutionController': ('62799712', 'Двойной тап — сбросить очки характеристик и умений'),
        'UseTerrorTokenAct1Controller': ('767eaea7', 'Двойной тап — ввергнуть ужас в 1-й акт'),
        'UseTerrorTokenAct2Controller': ('7e2b5ab4', 'Двойной тап — ввергнуть ужас во 2-й акт'),
        'UseTerrorTokenAct3Controller': ('cbfd42bc', 'Двойной тап — ввергнуть ужас в 3-й акт'),
        'UseTerrorTokenAct4Controller': ('5839a722', 'Двойной тап — ввергнуть ужас в 4-й акт'),
        'UseTerrorTokenAct5Controller': ('f8700dbe', 'Двойной тап — ввергнуть ужас в 5-й акт'),
        'comparisonTooltipController': ('0d40bbad', ''),
        'hireComparisonTooltipController': ('f42afaf2', ''),
        'StrYouDiedConsole': ('2bf9961e', 'Вы погибли'),
        'ControllerInteractButton': ('54234e14', ''),
        'ControllerTitleScreenPrompt': ('1b3b06b3', 'Коснитесь экрана, чтобы начать'),
        'EnterCharacterName': ('74790c9d', ''),
        'ControllerCycleComparisonTooltips': ('3f128d2b', 'Коснитесь снова, чтобы сравнить с другим'),
        'SelectButtonNoteText': ('f2040580', ''),
        'MoviePlaybackHoldToSkip_switch': ('08664e40', ''),
    },
    'ui.json': {
        'LevelUpStatsNotificationController': ('96716426', 'Уровень повышен.'),
        'LevelUpNotificationController': ('96716426', 'Уровень повышен.'),
        'QuestLogNotificationController': ('aad0ef25', 'Задание обновлено.'),
        'MoviePlaybackHoldToSkip': ('8f05e751', ''),
    },
}

KO = {
    'ui-controller.json': {
        'UseItemController': ('9f6ea691', '두 번 탭하여 사용'),
        'OpenHoradricCubeController': ('395d1d8b', '두 번 탭하여 열기'),
        'ReadScrollController': ('10a460a7', '두 번 탭하여 읽기'),
        'UsePotionOfLifeController': ('4a202757', '두 번 탭하여 생명력 영구적으로 20 추가하기'),
        'UseBookOfSkillController': ('f4d729a8', '두 번 탭하여 원하는 기술 습득하기'),
        'UseScrollOfResistanceController': ('097b145c', '두 번 탭하여 시전'),
        'UseTokenOfAbsolutionController': ('26eaa9f9', '두 번 탭하여 능력치/기술 포인트 초기화'),
        'UseTerrorTokenAct1Controller': ('389c4cc6', '두 번 탭하여 1막에 공포의 마수 뻗기'),
        'UseTerrorTokenAct2Controller': ('7a18b7cd', '두 번 탭하여 2막에 공포의 마수 뻗기'),
        'UseTerrorTokenAct3Controller': ('5d603b60', '두 번 탭하여 3막에 공포의 마수 뻗기'),
        'UseTerrorTokenAct4Controller': ('72a0b14d', '두 번 탭하여 4막에 공포의 마수 뻗기'),
        'UseTerrorTokenAct5Controller': ('fa51c7d5', '두 번 탭하여 5막에 공포의 마수 뻗기'),
        'comparisonTooltipController': ('d111fdb3', ''),
        'hireComparisonTooltipController': ('d111fdb3', ''),
        'StrYouDiedConsole': ('bc6a629b', '죽었습니다.'),
        'ControllerInteractButton': ('54234e14', ''),
        'ControllerTitleScreenPrompt': ('95b73988', '화면을 탭하여 시작'),
        'EnterCharacterName': ('6046f65e', ''),
        'ControllerCycleComparisonTooltips': ('a1ce13cb', '다시 탭하여 다른 아이템과 비교'),
        'SelectButtonNoteText': ('7f42b887', ''),
        'MoviePlaybackHoldToSkip_switch': ('9c102e1d', ''),
    },
    'ui.json': {
        'LevelUpStatsNotificationController': ('615ba7bd', '레벨이 올랐습니다.'),
        'LevelUpNotificationController': ('5f712271', '레벨이 올랐습니다.'),
        'QuestLogNotificationController': ('62166da0', '퀘스트가 갱신되었습니다.'),
        'MoviePlaybackHoldToSkip': ('9c102e1d', ''),
    },
}


DE = {
    'ui-controller.json': {
        'UseItemController': ('c5512a63', 'Doppeltippen zum Benutzen'),
        'OpenHoradricCubeController': ('08b827e1', 'Doppeltippen zum Öffnen'),
        'ReadScrollController': ('ff287f36', 'Doppeltippen zum Lesen'),
        'UsePotionOfLifeController': ('f7c6ea8e', 'Doppeltippen, um permanent 20 Leben zu erhalten'),
        'UseBookOfSkillController': ('5cd61408', 'Doppeltippen, um einen Fertigkeitspunkt zu erhalten'),
        'UseScrollOfResistanceController': ('ac1e7fa0', 'Doppeltippen zum Einsetzen'),
        'UseTokenOfAbsolutionController': ('1de7afa9', 'Doppeltippen, um Werte und Fertigkeitspunkte zurückzusetzen'),
        'UseTerrorTokenAct1Controller': ('37fb8814', 'Doppeltippen zum Terrorisieren (Akt 1)'),
        'UseTerrorTokenAct2Controller': ('c3c2782c', 'Doppeltippen zum Terrorisieren (Akt 2)'),
        'UseTerrorTokenAct3Controller': ('25b94898', 'Doppeltippen zum Terrorisieren (Akt 3)'),
        'UseTerrorTokenAct4Controller': ('a8bc8630', 'Doppeltippen zum Terrorisieren (Akt 4)'),
        'UseTerrorTokenAct5Controller': ('ff91e2e7', 'Doppeltippen zum Terrorisieren (Akt 5)'),
        'comparisonTooltipController': ('b40c62bb', ''),
        'hireComparisonTooltipController': ('40ed633f', ''),
        'StrYouDiedConsole': ('4a583ec1', 'Ihr seid gestorben.'),
        'ControllerInteractButton': ('54234e14', ''),
        'ControllerTitleScreenPrompt': ('8aa32708', 'Tippe auf den Bildschirm, um zu beginnen'),
        'EnterCharacterName': ('b0253e79', ''),
        'ControllerCycleComparisonTooltips': ('fa0ed51b', 'Erneut tippen, um mit dem anderen zu vergleichen'),
        'SelectButtonNoteText': ('21dc449a', ''),
        'MoviePlaybackHoldToSkip_switch': ('b75b2a19', ''),
    },
    'ui.json': {
        'LevelUpStatsNotificationController': ('f63865af', 'Ihr seid aufgestiegen.'),
        'LevelUpNotificationController': ('22706071', 'Ihr seid aufgestiegen.'),
        'QuestLogNotificationController': ('ea6b35a2', 'Quest wurde aktualisiert.'),
        'MoviePlaybackHoldToSkip': ('b75b2a19', ''),
    },
}

PT = {
    'ui-controller.json': {
        'UseItemController': ('fe58f6b2', 'Toque duas vezes para usar'),
        'OpenHoradricCubeController': ('afa25395', 'Toque duas vezes para abrir'),
        'ReadScrollController': ('436612cb', 'Toque duas vezes para ler'),
        'UsePotionOfLifeController': ('d25575ff', 'Toque duas vezes para adicionar 20 pontos de vida permanentemente'),
        'UseBookOfSkillController': ('1c65ad05', 'Toque duas vezes para aprender uma habilidade à sua escolha'),
        'UseScrollOfResistanceController': ('9e729a79', 'Toque duas vezes para lançar'),
        'UseTokenOfAbsolutionController': ('10c01129', 'Toque duas vezes para redefinir pontos de habilidades/atributos'),
        'UseTerrorTokenAct1Controller': ('b74b6353', 'Toque duas vezes para aterrorizar o Ato 1'),
        'UseTerrorTokenAct2Controller': ('055782ed', 'Toque duas vezes para aterrorizar o Ato 2'),
        'UseTerrorTokenAct3Controller': ('9c07fd7c', 'Toque duas vezes para aterrorizar o Ato 3'),
        'UseTerrorTokenAct4Controller': ('7a53f852', 'Toque duas vezes para aterrorizar o Ato 4'),
        'UseTerrorTokenAct5Controller': ('45b4825a', 'Toque duas vezes para aterrorizar o Ato 5'),
        'comparisonTooltipController': ('6ec5c0d6', ''),
        'hireComparisonTooltipController': ('6ec5c0d6', ''),
        'StrYouDiedConsole': ('5205feea', 'Você morreu'),
        'ControllerInteractButton': ('54234e14', ''),
        'ControllerTitleScreenPrompt': ('2bcda740', 'Toque na tela para começar'),
        'EnterCharacterName': ('d8fc6117', ''),
        'ControllerCycleComparisonTooltips': ('138181e2', 'Toque de novo para comparar com o outro'),
        'SelectButtonNoteText': ('42e98096', ''),
        'MoviePlaybackHoldToSkip_switch': ('b4969597', ''),
    },
    'ui.json': {
        'LevelUpStatsNotificationController': ('1f8aa3c8', 'Você subiu de nível.'),
        'LevelUpNotificationController': ('2801d211', 'Você subiu de nível.'),
        'QuestLogNotificationController': ('a9e68f30', 'A missão foi atualizada.'),
        'MoviePlaybackHoldToSkip': ('b4969597', ''),
    },
}
EXTRA = (('enUS', EN), ('esES', ES), ('esMX', MX), ('ruRU', RU), ('koKR', KO), ('deDE', DE), ('ptBR', PT))







FILE_ONLY = {'strchratr', 'strchrrat'}

def build_sidecar(romfs):

    out = []
    for name, items in TABLE.items():
        path, text = touch_layout.read_layout(romfs, None, suffix='/lng/strings/' + name)
        for key, (old, new) in items.items():
            if key.startswith('MoviePlaybackHoldToSkip') and os.environ.get('MOVIE_FFMPEG') == '1': continue
            if key in FILE_ONLY: continue
            m = re.search(r'"Key":\s*"%s",.*?"zhCN":\s*("(?:[^"\\]|\\.)*")' % re.escape(key), text, re.S)
            if not m: raise SystemExit('touch_strings：%s 里没有键 %s' % (name, key))
            cur = json.loads(m.group(1))
            if hashlib.sha1(cur.encode('utf-8')).hexdigest()[:8] != old: raise SystemExit('touch_strings：%s 的原文变了（校验 %s，现行「%s」）——原包换了版本，核过再改表' % (key, old, cur))
            out.append((key, old, new))




        for lang, tab in EXTRA:
            for key, (old, new) in tab.get(name, {}).items():
                if key.startswith('MoviePlaybackHoldToSkip') and os.environ.get('MOVIE_FFMPEG') == '1': continue
                m = re.search(r'"Key":\s*"%s",.*?"%s":\s*("(?:[^"\\]|\\.)*")' % (re.escape(key), lang), text, re.S)
                if not m: raise SystemExit('touch_strings：%s 里没有键 %s 的 %s' % (name, key, lang))
                cur = json.loads(m.group(1))
                if hashlib.sha1(cur.encode('utf-8')).hexdigest()[:8] != old: raise SystemExit('touch_strings：%s 的 %s 原文变了（校验 %s，现行「%s」）——原包换了版本，核过再改表' % (key, lang, old, cur))
                if (key, old, new) not in out: out.append((key, old, new))
    return out

def build(romfs, outdir):

    repl = {}; total = 0
    for name, items in TABLE.items():
        path, text = touch_layout.read_layout(romfs, None, suffix='/lng/strings/' + name)
        if not any(k in FILE_ONLY for k in items): continue
        for key, (old, new) in items.items():
            if key.startswith('MoviePlaybackHoldToSkip') and os.environ.get('MOVIE_FFMPEG') == '1': continue
            if key not in FILE_ONLY: continue
            m = re.search(r'"Key":\s*"%s",.*?"zhCN":\s*("(?:[^"\\]|\\.)*")' % re.escape(key), text, re.S)
            if not m: raise SystemExit('touch_strings：%s 里没有键 %s' % (name, key))
            cur = json.loads(m.group(1))
            if hashlib.sha1(cur.encode('utf-8')).hexdigest()[:8] != old: raise SystemExit('touch_strings：%s 的原文变了（校验 %s，现行「%s」）——原包换了版本，核过再改表' % (key, old, cur))
            text = text[:m.start(1)] + json.dumps(new, ensure_ascii=False) + text[m.end(1):]; total += 1
        o = os.path.join(outdir, 'str_' + name); open(o, 'w', encoding='utf-8-sig', newline='').write(text); repl[path.lower()] = o
    return repl, total
