#!/bin/bash





















set -e
root=$(readlink -f "$(dirname "$0")/../../..")
src=dm/android/app/src/main/java/com/xiaoboluo/d2r
cmd() {
  echo "set -e; o=\$(mktemp -d)
  python3 $1/dm/android/tests/run_signing_test.py
  javac -encoding UTF-8 -d \$o/rm $1/$src/RecipeModel.java $1/$src/RecipeStock.java $1/$src/RecipeMatch.java $1/$src/RecipeInsert.java $1/$src/RecipeTerms.java $1/dm/android/tests/RecipeMatchTest.java
  java -cp \$o/rm com.xiaoboluo.d2r.RecipeMatchTest
  javac -encoding UTF-8 -d \$o/rs $1/$src/RecipeModel.java $1/$src/RecipeStock.java $1/$src/RecipeMatch.java $1/$src/RecipeTerms.java $1/$src/RecipeStyle.java $1/$src/RecipeSearchInput.java $1/$src/GameStyleData.java $1/dm/android/tests/RecipeStyleTest.java $1/dm/android/tests/RecipeSearchInputTest.java $1/dm/android/tests/RecipeTermsTest.java
  java -cp \$o/rs com.xiaoboluo.d2r.RecipeStyleTest
  java -cp \$o/rs com.xiaoboluo.d2r.RecipeSearchInputTest
  java -cp \$o/rs com.xiaoboluo.d2r.RecipeTermsTest
  javac -encoding UTF-8 -d \$o/rp $1/$src/RecipeModel.java $1/$src/RecipePreview.java $1/dm/android/tests/RecipePreviewTest.java
  java -cp \$o/rp com.xiaoboluo.d2r.RecipePreviewTest
  javac -encoding UTF-8 -d \$o/st $1/$src/DisplayGate.java $1/dm/android/tests/DisplayGateTest.java $1/$src/StatusModel.java $1/dm/android/tests/StatusModelTest.java
  java -cp \$o/st com.xiaoboluo.d2r.DisplayGateTest
  java -cp \$o/st com.xiaoboluo.d2r.StatusModelTest
  javac -encoding UTF-8 -d \$o/fx $1/$src/EffectFormat.java $1/$src/EffectLayout.java $1/$src/EffectDetail.java $1/$src/Plural.java $1/$src/EffectCalc.java $1/$src/StatusPlace.java $1/$src/PendingFingers.java $1/$src/EffectOrder.java $1/dm/android/tests/EffectFormatTest.java $1/dm/android/tests/EffectLayoutTest.java $1/dm/android/tests/EffectDetailTest.java $1/dm/android/tests/EffectCalcTest.java $1/dm/android/tests/PendingFingersTest.java $1/dm/android/tests/EffectOrderTest.java
  java -cp \$o/fx com.xiaoboluo.d2r.EffectFormatTest
  java -cp \$o/fx com.xiaoboluo.d2r.EffectDetailTest
  java -cp \$o/fx com.xiaoboluo.d2r.EffectCalcTest
  java -cp \$o/fx com.xiaoboluo.d2r.EffectLayoutTest
  java -cp \$o/fx com.xiaoboluo.d2r.PendingFingersTest
  java -cp \$o/fx com.xiaoboluo.d2r.EffectOrderTest
  javac -encoding UTF-8 -d \$o/pad $1/$src/PadHold.java $1/$src/PadGate.java $1/$src/PadNav.java $1/$src/MenuFocus.java $1/dm/android/tests/PadHoldTest.java $1/dm/android/tests/PadNavTest.java $1/dm/android/tests/MenuFocusTest.java
  java -cp \$o/pad com.xiaoboluo.d2r.PadHoldTest
  java -cp \$o/pad com.xiaoboluo.d2r.PadNavTest
  java -cp \$o/pad com.xiaoboluo.d2r.MenuFocusTest
  javac -encoding UTF-8 -d \$o/qm $1/$src/QuickLayout.java $1/$src/QuickModel.java $1/$src/GameStyleGeometry.java $1/$src/GameStyleData.java $1/dm/android/tests/QuickLayoutTest.java $1/dm/android/tests/QuickModelTest.java
  java -cp \$o/qm com.xiaoboluo.d2r.QuickLayoutTest
  java -cp \$o/qm com.xiaoboluo.d2r.QuickModelTest
  javac -encoding UTF-8 -cp \$o/qm -d \$o/qm $1/$src/CombatModel.java $1/$src/GameLang.java $1/$src/CombatLayout.java $1/$src/CombatGraph.java $1/$src/FightLog.java $1/$src/StreakLog.java $1/dm/android/tests/CombatModelTest.java $1/dm/android/tests/CombatLayoutTest.java $1/dm/android/tests/FightLogTest.java $1/dm/android/tests/StreakLogTest.java $1/$src/DpsLog.java $1/dm/android/tests/DpsLogTest.java
  java -cp \$o/qm com.xiaoboluo.d2r.CombatModelTest
  java -cp \$o/qm com.xiaoboluo.d2r.CombatLayoutTest
  javac -encoding UTF-8 -cp \$o/qm -d \$o/qm $1/$src/BookModel.java $1/$src/BookLayout.java $1/$src/BookRibbon.java $1/$src/BookStats.java $1/$src/PadNav.java $1/$src/BookText.java $1/$src/Plural.java $1/dm/android/tests/BookModelTest.java $1/dm/android/tests/BookLayoutTest.java $1/dm/android/tests/BookRibbonTest.java $1/dm/android/tests/BookStatsTest.java
  java -cp \$o/qm com.xiaoboluo.d2r.BookModelTest
  java -cp \$o/qm com.xiaoboluo.d2r.BookStatsTest $1
  java -cp \$o/qm com.xiaoboluo.d2r.BookLayoutTest
  java -cp \$o/qm com.xiaoboluo.d2r.BookRibbonTest
  java -cp \$o/qm com.xiaoboluo.d2r.FightLogTest
  java -cp \$o/qm com.xiaoboluo.d2r.DpsLogTest
  java -cp \$o/qm com.xiaoboluo.d2r.StreakLogTest
  javac -encoding UTF-8 -d \$o/fq $1/$src/FeatQueue.java $1/dm/android/tests/FeatQueueTest.java
  java -cp \$o/fq com.xiaoboluo.d2r.FeatQueueTest
  javac -encoding UTF-8 -d \$o/fm $1/$src/FeatsModel.java $1/$src/FeatsLayout.java $1/$src/PadNav.java $1/dm/android/tests/FeatsModelTest.java $1/dm/android/tests/FeatsLayoutTest.java
  java -cp \$o/fm com.xiaoboluo.d2r.FeatsModelTest
  java -cp \$o/fm com.xiaoboluo.d2r.FeatsLayoutTest
  javac -encoding UTF-8 -d \$o/bp $1/$src/BuildModel.java $1/$src/BuildPlan.java $1/dm/android/tests/BuildPlanTest.java
  java -cp \$o/bp com.xiaoboluo.d2r.BuildPlanTest $1/dm/android/tests
  javac -encoding UTF-8 -d \$o/gp $1/$src/BuildModel.java $1/$src/GearPreview.java $1/dm/android/tests/GearPreviewTest.java
  java -cp \$o/gp com.xiaoboluo.d2r.GearPreviewTest
  javac -encoding UTF-8 -d \$o/ov $1/$src/OverviewFlow.java $1/dm/android/tests/OverviewFlowTest.java
  java -cp \$o/ov com.xiaoboluo.d2r.OverviewFlowTest
  javac -encoding UTF-8 -d \$o/gs $1/$src/GameStyleData.java $1/$src/GameStyleGeometry.java $1/dm/android/tests/GameStyleDataTest.java $1/dm/android/tests/GameStyleGeometryTest.java
  java -cp \$o/gs com.xiaoboluo.d2r.GameStyleDataTest
  java -cp \$o/gs com.xiaoboluo.d2r.GameStyleGeometryTest
  javac -encoding UTF-8 -d \$o/dn $1/$src/DamageMotion.java $1/$src/StatusText.java $1/dm/android/tests/DamageMotionTest.java $1/dm/android/tests/StatusTextTest.java
  java -cp \$o/dn com.xiaoboluo.d2r.DamageMotionTest
  java -cp \$o/dn com.xiaoboluo.d2r.StatusTextTest
  javac -encoding UTF-8 -d \$o/hud $1/$src/StatusPlace.java $1/$src/LevelLabel.java $1/dm/android/tests/StatusPlaceTest.java $1/dm/android/tests/LevelLabelTest.java
  java -cp \$o/hud com.xiaoboluo.d2r.StatusPlaceTest
  javac -encoding UTF-8 -d \$o/gr $1/$src/GameRect.java $1/dm/android/tests/GameRectTest.java
  java -cp \$o/gr com.xiaoboluo.d2r.GameRectTest
  java -cp \$o/hud com.xiaoboluo.d2r.LevelLabelTest
  javac -encoding UTF-8 -d \$o/gl $1/$src/GameLang.java $1/dm/android/tests/GameLangTest.java
  java -cp \$o/gl com.xiaoboluo.d2r.GameLangTest
  javac -encoding UTF-8 -d \$o/gloss $1/$src/GlossaryModel.java $1/$src/GlossaryCalc.java $1/$src/GlossaryLive.java $1/$src/EffectDetail.java $1/$src/Plural.java $1/$src/EffectFormat.java $1/$src/EffectCalc.java $1/dm/android/tests/GlossaryTest.java
  java -cp \$o/gloss com.xiaoboluo.d2r.GlossaryTest
  javac -encoding UTF-8 -d \$o/ka $1/$src/KeepAliveStatus.java $1/$src/QuitRule.java $1/$src/BatteryAsk.java $1/dm/android/tests/KeepAliveStatusTest.java $1/dm/android/tests/QuitRuleTest.java $1/dm/android/tests/BatteryAskTest.java
  java -cp \$o/ka com.xiaoboluo.d2r.KeepAliveStatusTest $1
  javac -encoding UTF-8 -d \$o/bg $1/$src/BestGearModel.java $1/dm/android/tests/BestGearStoreTest.java
  java -cp \$o/bg com.xiaoboluo.d2r.BestGearStoreTest $1
  javac -encoding UTF-8 -d \$o/bp $1/$src/BestGearPages.java $1/dm/android/tests/BestGearPagesTest.java
  java -cp \$o/bp com.xiaoboluo.d2r.BestGearPagesTest
  javac -encoding UTF-8 -d \$o/be $1/$src/BestGearEquip.java $1/dm/android/tests/BestGearEquipTest.java
  java -cp \$o/be com.xiaoboluo.d2r.BestGearEquipTest
  javac -encoding UTF-8 -d \$o/pl $1/$src/Plural.java $1/dm/android/tests/PluralTest.java
  java -cp \$o/pl com.xiaoboluo.d2r.PluralTest
  javac -encoding UTF-8 -d \$o/dl $1/$src/DotLayout.java $1/dm/android/tests/DotLayoutTest.java
  java -cp \$o/dl com.xiaoboluo.d2r.DotLayoutTest
  javac -encoding UTF-8 -d \$o/dr $1/$src/DotLayout.java $1/$src/DotRows.java $1/dm/android/tests/stubs/Context.java $1/dm/android/tests/stubs/ActivityManager.java $1/dm/android/tests/stubs/ApplicationExitInfo.java $1/dm/android/tests/stubs/ApplicationInfo.java $1/dm/android/tests/stubs/PackageInfo.java $1/dm/android/tests/stubs/PackageManager.java $1/dm/android/tests/stubs/Canvas.java $1/dm/android/tests/stubs/RectF.java $1/dm/android/tests/stubs/AppText.java $1/dm/android/tests/stubs/R.java $1/dm/android/tests/DotRowsTest.java
  java -cp \$o/dr com.xiaoboluo.d2r.DotRowsTest
  javac -encoding UTF-8 -d \$o/ip $1/$src/InvPictures.java $1/dm/android/tests/InvPicturesTest.java
  java -cp \$o/ip com.xiaoboluo.d2r.InvPicturesTest
  javac -encoding UTF-8 -d \$o/drop $1/$src/BookJson.java $1/$src/BookData.java $1/$src/DropCalc.java $1/dm/android/tests/DropCalcTest.java
  java -cp \$o/drop com.xiaoboluo.d2r.DropCalcTest
  javac -encoding UTF-8 -cp \$o/qm -d \$o/qm $1/$src/BookJson.java $1/$src/BookData.java $1/$src/DropCalc.java $1/$src/BookLoot.java $1/dm/android/tests/BookLootTest.java
  java -cp \$o/qm com.xiaoboluo.d2r.BookLootTest
  javac -encoding UTF-8 -d \$o/mp $1/$src/MonPictures.java $1/$src/MonCache.java $1/$src/MonLoader.java $1/dm/android/tests/MonPicturesTest.java
  java -cp \$o/mp com.xiaoboluo.d2r.MonPicturesTest
  java -cp \$o/ka com.xiaoboluo.d2r.BatteryAskTest
  java -cp \$o/ka com.xiaoboluo.d2r.QuitRuleTest
  javac -encoding UTF-8 -d \$o/xfer $1/$src/SaveTransfer.java $1/dm/android/tests/SaveTransferTest.java
  java -cp \$o/xfer com.xiaoboluo.d2r.SaveTransferTest $1/dm/android/tests/looks-host.swlooks $1/dm/android/tests/looks-host-v2.swlooks
  javac -encoding UTF-8 -d \$o/storage $1/$src/SharedStorage.java $1/$src/SaveTransfer.java $1/dm/android/tests/SharedStorageTest.java
  java -cp \$o/storage com.xiaoboluo.d2r.SharedStorageTest
  javac -encoding UTF-8 -d \$o/pb $1/$src/PackProgress.java $1/$src/PackSource.java $1/$src/PackJournal.java $1/$src/PackBuildRun.java $1/$src/PackBuildState.java $1/$src/PackBuildControl.java $1/$src/PackDocs.java $1/$src/PackRows.java $1/$src/PackSpace.java $1/$src/PackBuildJob.java $1/$src/PackInPlace.java $1/$src/PackSteps.java $1/$src/PackPlan.java $1/$src/PackSweep.java $1/$src/PackLog.java $1/$src/PackInstall.java $1/$src/PackGeneration.java $1/$src/PackLock.java $1/$src/PackSession.java $1/$src/PackEntry.java $1/$src/DriverLibrary.java $1/$src/DriverZip.java $1/dm/android/tests/stubs/PackReader.java $1/dm/android/tests/stubs/AppArt.java $1/dm/android/tests/PackProgressTest.java $1/dm/android/tests/PackSourceTest.java $1/dm/android/tests/PackJournalTest.java $1/dm/android/tests/PackBuildRunTest.java $1/dm/android/tests/PackPickTest.java $1/dm/android/tests/PackBuildJobTest.java $1/dm/android/tests/PackStepsTest.java $1/dm/android/tests/PackPlanTest.java $1/dm/android/tests/PackSweepTest.java $1/dm/android/tests/PackLogTest.java $1/dm/android/tests/PackInstallTest.java $1/dm/android/tests/PackSessionTest.java $1/dm/android/tests/PackEntryTest.java
  java -cp \$o/pb com.xiaoboluo.d2r.PackProgressTest
  java -cp \$o/pb com.xiaoboluo.d2r.PackSourceTest
  java -cp \$o/pb com.xiaoboluo.d2r.PackJournalTest
  java -cp \$o/pb com.xiaoboluo.d2r.PackBuildRunTest
  java -cp \$o/pb com.xiaoboluo.d2r.PackPickTest
  java -cp \$o/pb com.xiaoboluo.d2r.PackBuildJobTest
  java -Ddm.root=$1 -cp \$o/pb com.xiaoboluo.d2r.PackStepsTest
  java -cp \$o/pb com.xiaoboluo.d2r.PackPlanTest
  java -cp \$o/pb com.xiaoboluo.d2r.PackSweepTest
  java -cp \$o/pb com.xiaoboluo.d2r.PackLogTest
  java -Ddm.root=$1 -cp \$o/pb com.xiaoboluo.d2r.PackInstallTest
  java -cp \$o/pb com.xiaoboluo.d2r.PackSessionTest
  java -cp \$o/pb com.xiaoboluo.d2r.PackEntryTest
  javac -encoding UTF-8 -cp \$o/pb -d \$o/ph $1/dm/android/app/src/player/java/com/xiaoboluo/d2r/TestHooks.java $1/dm/android/tests/stubs/Context.java $1/dm/android/tests/stubs/ApplicationInfo.java $1/dm/android/tests/stubs/PackageManager.java $1/dm/android/tests/stubs/PackageInfo.java $1/dm/android/tests/stubs/ActivityManager.java $1/dm/android/tests/stubs/ApplicationExitInfo.java $1/dm/android/tests/stubs/Canvas.java $1/dm/android/tests/stubs/RectF.java $1/dm/android/tests/stubs/Uri.java $1/dm/android/tests/stubs/View.java $1/dm/android/tests/stubs/Paint.java $1/dm/android/tests/PlayerHooksTest.java
  java -Ddm.root=$1 -cp \$o/pb:\$o/ph com.xiaoboluo.d2r.PlayerHooksTest
  javac -encoding UTF-8 -d \$o/drv $1/$src/DriverGuard.java $1/$src/DriverZip.java $1/$src/DriverElf.java $1/$src/DriverLibrary.java $1/$src/DriverStore.java $1/dm/android/tests/stubs/AppArt.java $1/dm/android/tests/stubs/AppText.java $1/dm/android/tests/stubs/Build.java $1/dm/android/tests/stubs/BuildConfig.java $1/dm/android/tests/stubs/Context.java $1/dm/android/tests/stubs/DmActivity.java $1/dm/android/tests/stubs/Icon.java $1/dm/android/tests/stubs/Intent.java $1/dm/android/tests/stubs/PackageInfo.java $1/dm/android/tests/stubs/PackageManager.java $1/dm/android/tests/stubs/ApplicationInfo.java $1/dm/android/tests/stubs/R.java $1/dm/android/tests/stubs/ShortcutInfo.java $1/dm/android/tests/stubs/ShortcutManager.java $1/dm/android/tests/stubs/Os.java $1/dm/android/tests/stubs/OsConstants.java $1/dm/android/tests/stubs/ErrnoException.java $1/dm/android/tests/stubs/ActivityManager.java $1/dm/android/tests/stubs/ApplicationExitInfo.java $1/dm/android/tests/stubs/Process.java $1/dm/android/tests/DriverGuardTest.java $1/dm/android/tests/DriverZipTest.java $1/dm/android/tests/DriverElfTest.java $1/dm/android/tests/DriverLibraryTest.java $1/dm/android/tests/DriverStoreTest.java $1/dm/android/tests/DriverTextTest.java
  java -cp \$o/drv com.xiaoboluo.d2r.DriverGuardTest
  java -cp \$o/drv com.xiaoboluo.d2r.DriverZipTest
  java -cp \$o/drv com.xiaoboluo.d2r.DriverElfTest
  java -cp \$o/drv com.xiaoboluo.d2r.DriverLibraryTest
  java -cp \$o/drv com.xiaoboluo.d2r.DriverStoreTest
  java -cp \$o/drv com.xiaoboluo.d2r.DriverTextTest $1
  rm -rf \$o"
}


if command -v python3 > /dev/null && python3 -c 'import PIL' 2> /dev/null; then
  for t in test_profile_hd test_gen_style test_check_style test_dcc test_effect_art test_effect_data test_check_glossary test_inv_pics test_book_data test_book_text; do python3 "$root/dm/android/tests/$t.py"; done
else echo "run.sh: python3 with Pillow not found, pack tool tests skipped"; fi


if command -v make > /dev/null && command -v c++ > /dev/null; then make -s -C "$root/dm/android/tools/monpic" test
else echo "run.sh: make or a C++ compiler not found, picture library tests skipped"; fi
if command -v python3 > /dev/null && python3 -c 'import PIL, numpy' 2> /dev/null && { command -v c++ > /dev/null || command -v clang++ > /dev/null || [ -n "${DM_MONPIC_LIB:-}" ]; }; then
  python3 "$root/dm/android/tests/test_mon_pics.py"
else echo "run.sh: python3 with Pillow and numpy or a C/C++ compiler not found, enemy picture tool test skipped"; fi
python3 "$root/dm/android/tests/test_release_tools.py"
python3 "$root/dm/android/tests/test_gen_strings.py"
python3 "$root/dm/android/tests/test_gen_recipe_card.py"

if command -v cc > /dev/null; then
  t=$(mktemp -d)
  cc -O1 -I "$root" -o "$t/fv" "$root/dm/android/tests/test_front_view.c" -lm
  "$t/fv"
  cc -O1 -I "$root" -o "$t/ts" "$root/dm/android/tests/test_touch_snapshot.c" -lm
  "$t/ts" > "$t/ts.log"
  tail -1 "$t/ts.log"

  cc -std=gnu11 -O1 -Wall -Wno-unused-function -fno-strict-aliasing -D_GNU_SOURCE -D__ANDROID__ -I "$root/dm/android/tests/fake_aaudio" -I "$root/dm/android/compat/include" \
    -I "$root/dm/host" -o "$t/ar" "$root/dm/android/tests/test_audio_reopen.c" -lpthread
  "$t/ar" > "$t/ar.log" 2>&1 || { grep -E '^(PASS|FAIL)' "$t/ar.log"; exit 1; }
  tail -1 "$t/ar.log"
  rm -rf "$t"
else echo "run.sh: no C compiler, native bridge tests skipped"; fi
if command -v javac > /dev/null; then exec sh -c "$(cmd "$root")"; fi
exec docker run --rm --user "$(id -u):$(id -g)" -v "$root:/src:ro" "${DM_ANDROID_BUILD_IMAGE:-dm-android-build:1}" sh -c "$(cmd /src)"
