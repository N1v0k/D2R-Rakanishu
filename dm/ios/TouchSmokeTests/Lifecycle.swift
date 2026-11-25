import XCTest







final class LifecycleSmoke: XCTestCase {
    func testBackgroundAndReturn() throws {
        continueAfterFailure = false
        let app = XCUIApplication()
        app.launchArguments = ["-mode", "boot"]
        app.launchEnvironment = ["DM_SAVE_FRESH":"warlock", "DM_UIMSG":"1", "DM_AUDIO":"1",
            "DM_KEYS":"X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,DOWN@OpenPanel:CharacterSelectPanel>UpdateCharacterInfo,A@UpdateCharacterInfo%1>CharacterSelect:SelectCharacter,A@CharacterSelect:SelectCharacter>FrontEndNavigation:ToGame"]
        app.launch()
        defer { app.terminate() }
        sleep(25)
        XCTAssertEqual(app.state, .runningForeground)
        XCUIDevice.shared.press(.home)
        sleep(30)
        XCTAssertNotEqual(app.state, .notRunning, "后台期间进程被系统结束")
        app.activate()
        XCTAssertTrue(app.wait(for: .runningForeground, timeout: 10))
        sleep(20)
        XCTAssertEqual(app.state, .runningForeground)
    }





    private static let enterKeys = "X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,DOWN@OpenPanel:CharacterSelectPanel>UpdateCharacterInfo,A@UpdateCharacterInfo%1>CharacterSelect:SelectCharacter,A@CharacterSelect:SelectCharacter>FrontEndNavigation:ToGame"
    func testSwitcherKill() throws {
        continueAfterFailure = false
        let app = XCUIApplication()
        app.launchArguments = ["-mode", "boot"]
        app.launchEnvironment = ["DM_SAVE_FRESH":"warlock", "DM_UIMSG":"1", "DM_KEYS":Self.enterKeys, "DM_TEST_GIVE":"r30@200", "DM_TEST_GIVE_INV":"1"]
        app.launch()
        sleep(25)
        XCTAssertEqual(app.state, .runningForeground)
        let from = app.coordinate(withNormalizedOffset: CGVector(dx: 0.5, dy: 0.998)), to = app.coordinate(withNormalizedOffset: CGVector(dx: 0.5, dy: 0.55))
        from.press(forDuration: 0.05, thenDragTo: to, withVelocity: .slow, thenHoldForDuration: 1.0)
        sleep(3)
        app.terminate()
    }


    func testReenter() throws {
        continueAfterFailure = false
        let app = XCUIApplication()
        app.launchArguments = ["-mode", "boot"]
        app.launchEnvironment = ["DM_UIMSG":"1", "DM_KEYS":Self.enterKeys, "DM_STATE":"lc_state.jsonl"]
        app.launch()
        sleep(30)
        app.terminate()
    }
}
