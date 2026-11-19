import XCTest



final class TouchSmoke: XCTestCase {
    func testJoystickAndPanelRoundTrip() throws {
        continueAfterFailure = false
        let app = XCUIApplication()
        app.launchArguments = ["-mode", "boot"]
        app.launchEnvironment = ["DM_SAVE_FRESH":"warlock", "DM_WATCH":"50", "DM_OPMODE":"0",
            "DM_KEYS":"A@OpenPanel:MainMenuButtonRibbon%0.7>FrontEndNavigation:ToGame",
            "DM_TOUCH_CONTROL_LOG":"1", "DM_UIMSG":"touch_uimsg.jsonl", "DM_C2S_LOG":"touch_c2s.jsonl",
            "DM_TICKS":"touch_ticks.jsonl", "DM_TICKS_FULL":"1",
            "DM_TOUCH_SHOT":"22:touch_overlay.png", "DM_SCREEN_SHOT":"22:touch_window.png"]
        app.launch()
        defer { app.terminate() }
        let surface = app.otherElements["dm_touch_surface"]
        XCTAssertTrue(surface.waitForExistence(timeout:15))
        XCTAssertTrue(XCTNSPredicateExpectation(predicate:NSPredicate(format:"value BEGINSWITH %@", "game|"), object:surface).wait(25))


        let box = surface.frame
        let w = box.width, h = box.height
        func at(_ x:CGFloat,_ y:CGFloat) -> XCUICoordinate {
            return surface.coordinate(withNormalizedOffset:CGVector(dx:x/w,dy:y/h))
        }
        at(90,h-100).press(forDuration:0.15, thenDragTo:at(150,h-100))
        at(90,h-100).press(forDuration:0.15, thenDragTo:at(30,h-100))


        at(w*0.55,h*0.25).tap()
        let sample = (surface.value as? String) ?? ""
        XCTAssertTrue(sample.hasPrefix("game|"))
        let fields = Dictionary(uniqueKeysWithValues:sample.split(separator:"|").dropFirst().map { part -> (String,String) in
            let pair=part.split(separator:"=",maxSplits:1);return (String(pair[0]),String(pair[1]))
        })
        XCTAssertGreaterThanOrEqual(Int(fields["max"] ?? "0") ?? 0,32000)
        XCTAssertLessThanOrEqual(Int(fields["min"] ?? "0") ?? 0,-32000)
        XCTAssertEqual(fields["buttons"],"0");XCTAssertEqual(fields["lx"],"0");XCTAssertEqual(fields["ly"],"0")
        XCTAssertEqual(fields["a"],"0", "空白触点不得生成主技能A")


        at(w/2+120,h-30).tap()
        XCTAssertTrue(XCTNSPredicateExpectation(predicate:NSPredicate(format:"value BEGINSWITH %@", "navigation|"), object:surface).wait(5))
        let panel = XCTAttachment(screenshot:app.screenshot()); panel.name="touch_inventory";panel.lifetime = .keepAlways;add(panel)


        at(w-56-112,h-82).tap()
        XCTAssertTrue(XCTNSPredicateExpectation(predicate:NSPredicate(format:"value BEGINSWITH %@", "game|"), object:surface).wait(5))
        let world = XCTAttachment(screenshot:app.screenshot());world.name="touch_game";world.lifetime = .keepAlways;add(world)


        XCTAssertTrue(XCTNSPredicateExpectation(predicate:NSPredicate(format:"value CONTAINS %@", "|shot=1"), object:surface).wait(25), "透明操作层写盘完成才结束")
        app.terminate()
    }
}
private extension XCTestExpectation {
    func wait(_ seconds:TimeInterval) -> Bool { XCTWaiter.wait(for:[self],timeout:seconds) == .completed }
}
