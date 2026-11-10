#import <CoreMotion/CoreMotion.h>
#import <UIKit/UIKit.h>

#include "IosPlatform.h"

#include <mutex>

#include "GameSettings.h"
#include "SensorData.h"

// iOS counterpart of the Android menus in MainActivity.java. There is no Exit button because iOS
// apps are closed by the system, not from inside the app.

namespace {

const CGFloat kStandardGravity = 9.81;

UIColor *rgba(CGFloat r, CGFloat g, CGFloat b, CGFloat a = 255) {
    return [UIColor colorWithRed:r / 255 green:g / 255 blue:b / 255 alpha:a / 255];
}

UIFont *monoFont(CGFloat size, UIFontWeight weight = UIFontWeightRegular) {
    return [UIFont monospacedSystemFontOfSize:size weight:weight];
}

UILabel *makeLabel(NSString *text, UIColor *color, CGFloat size, UIFontWeight weight) {
    UILabel *label = [[UILabel alloc] init];
    label.text = text;
    label.textColor = color;
    label.font = monoFont(size, weight);
    label.textAlignment = NSTextAlignmentCenter;
    label.numberOfLines = 0;
    return label;
}

UIStackView *makeStack(UILayoutConstraintAxis axis, CGFloat spacing, NSArray<UIView *> *views) {
    UIStackView *stack = [[UIStackView alloc] initWithArrangedSubviews:views];
    stack.axis = axis;
    stack.spacing = spacing;
    stack.alignment = UIStackViewAlignmentCenter;
    return stack;
}

} // namespace

/** Lets touches through wherever it has no control of its own. */
@interface SFPassthroughView : UIView
@end

@implementation SFPassthroughView
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hit = [super hitTest:point withEvent:event];
    return hit == self ? nil : hit;
}
@end

@interface SFMotion : NSObject
- (void)start;
- (void)stop;
@property(nonatomic, weak) UIView *view;
@end

@implementation SFMotion {
    CMMotionManager *_manager;
}

- (instancetype)init {
    if ((self = [super init])) {
        _manager = [[CMMotionManager alloc] init];
        _manager.deviceMotionUpdateInterval = 1.0 / 60.0;
    }
    return self;
}

- (void)start {
    if (!_manager.isDeviceMotionAvailable || _manager.isDeviceMotionActive) {
        return;
    }
    __weak SFMotion *weakSelf = self;
    [_manager startDeviceMotionUpdatesUsingReferenceFrame:CMAttitudeReferenceFrameXArbitraryCorrectedZVertical
                                                  toQueue:NSOperationQueue.mainQueue
                                              withHandler:^(CMDeviceMotion *motion, NSError *error) {
                                                  if (motion) {
                                                      [weakSelf publish:motion];
                                                  }
                                              }];
}

- (void)stop {
    [_manager stopDeviceMotionUpdates];
}

- (void)publish:(CMDeviceMotion *)motion {
    // Written to match Android's rotation vector, which OrientationTracker expects. A landscape-left
    // interface here is the same physical pose as Android's ROTATION_270.
    CMQuaternion q = motion.attitude.quaternion;
    float flip = self.view.window.windowScene.interfaceOrientation == UIInterfaceOrientationLandscapeLeft
                 ? -1.0f : 1.0f;

    std::lock_guard<std::mutex> lock(gSensorMutex);
    // Core Motion reports gravity in g with the opposite sign to Android's TYPE_GRAVITY.
    gSensorData.grav[0] = (float) (motion.gravity.x * -kStandardGravity);
    gSensorData.grav[1] = (float) (motion.gravity.y * -kStandardGravity);
    gSensorData.grav[2] = (float) (motion.gravity.z * -kStandardGravity);
    gSensorData.gyro[0] = (float) motion.rotationRate.x;
    gSensorData.gyro[1] = (float) motion.rotationRate.y;
    gSensorData.gyro[2] = (float) motion.rotationRate.z;
    gSensorData.orientation[0] = (float) q.x * flip;
    gSensorData.orientation[1] = (float) q.y * flip;
    gSensorData.orientation[2] = (float) q.z;
    gSensorData.orientation[3] = (float) q.w;
}
@end

@interface SFMenus : NSObject
- (instancetype)initWithHost:(UIView *)host;
@end

@implementation SFMenus {
    SFPassthroughView *_root;
    UIView *_mainMenu;
    UIView *_levelSelect;
    UIView *_workshop;
    UIView *_pause;
    UIView *_hud;
}

- (instancetype)initWithHost:(UIView *)host {
    if ((self = [super init])) {
        _root = [[SFPassthroughView alloc] initWithFrame:host.bounds];
        _root.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        [host addSubview:_root];

        _mainMenu = [self makeMainMenu];
        _levelSelect = [self makeLevelSelect];
        _workshop = [self makeWorkshop];
        _pause = [self makePause];
        _hud = [self makeHud];
        [self setState:GameState::MAIN_MENU];
    }
    return self;
}

- (void)setState:(GameState)state {
    applyGameState(state);
    _mainMenu.hidden = state != GameState::MAIN_MENU;
    _levelSelect.hidden = state != GameState::LEVEL_SELECT;
    _workshop.hidden = state != GameState::WORKSHOP;
    _pause.hidden = state != GameState::PAUSED;
    _hud.hidden = state != GameState::PLAYING;
}

- (UIButton *)buttonTitled:(NSString *)title
                     color:(UIColor *)color
                      size:(CGSize)size
                    action:(void (^)(void))action {
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:title forState:UIControlStateNormal];
    [button setTitleColor:color forState:UIControlStateNormal];
    [button setTitleColor:UIColor.grayColor forState:UIControlStateDisabled];
    button.titleLabel.font = monoFont(13, UIFontWeightBold);
    button.titleLabel.numberOfLines = 0;
    button.titleLabel.textAlignment = NSTextAlignmentCenter;
    button.backgroundColor = rgba(20, 25, 40, 180);
    [button.widthAnchor constraintEqualToConstant:size.width].active = YES;
    [button.heightAnchor constraintEqualToConstant:size.height].active = YES;
    if (action) {
        [button addAction:[UIAction actionWithHandler:^(__kindof UIAction *sender) {
            action();
        }]
            forControlEvents:UIControlEventTouchUpInside];
    } else {
        button.enabled = NO;
    }
    return button;
}

/** Adds a full-screen dimmed layer with content centered inside the safe area. */
- (UIView *)addScreen:(UIView *)content background:(UIColor *)background {
    UIView *screen = [[UIView alloc] initWithFrame:_root.bounds];
    screen.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    screen.backgroundColor = background;

    content.translatesAutoresizingMaskIntoConstraints = NO;
    [screen addSubview:content];
    UILayoutGuide *safe = screen.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [content.centerXAnchor constraintEqualToAnchor:safe.centerXAnchor],
        [content.centerYAnchor constraintEqualToAnchor:safe.centerYAnchor],
    ]];

    [_root addSubview:screen];
    return screen;
}

- (UIView *)gridOf:(NSArray<UIView *> *)buttons {
    return makeStack(UILayoutConstraintAxisVertical, 12, @[
        makeStack(UILayoutConstraintAxisHorizontal, 12, @[buttons[0], buttons[1]]),
        makeStack(UILayoutConstraintAxisHorizontal, 12, @[buttons[2], buttons[3]]),
    ]);
}

- (UIView *)makeMainMenu {
    __weak SFMenus *weakSelf = self;
    CGSize size = CGSizeMake(200, 56);
    UIView *play = [self buttonTitled:@"🚀 PLAY GAME" color:UIColor.cyanColor size:size action:^{
        [weakSelf setState:GameState::LEVEL_SELECT];
    }];
    UIView *workshop = [self buttonTitled:@"🛠️ SHIP WORKSHOP" color:UIColor.whiteColor size:size action:^{
        [weakSelf setState:GameState::WORKSHOP];
    }];
    UIView *settings = [self buttonTitled:@"⚙️ SETTINGS" color:UIColor.whiteColor size:size action:^{
        [weakSelf showSettings];
    }];

    UIStackView *content = makeStack(UILayoutConstraintAxisVertical, 6, @[
        makeLabel(@"STAR FLYER", UIColor.cyanColor, 32, UIFontWeightBold),
        makeLabel(@"SCI-FI FLIGHT SIMULATOR", UIColor.lightGrayColor, 12, UIFontWeightRegular),
        makeStack(UILayoutConstraintAxisHorizontal, 12, @[play, workshop]),
        settings,
    ]);
    [content setCustomSpacing:20 afterView:content.arrangedSubviews[1]];
    [content setCustomSpacing:12 afterView:content.arrangedSubviews[2]];
    return [self addScreen:content background:rgba(5, 10, 20, 160)];
}

- (UIView *)makeLevelSelect {
    __weak SFMenus *weakSelf = self;
    CGSize card = CGSizeMake(160, 96);
    UIView *level0 = [self buttonTitled:@"MISSION 0:\nDEBUG DEBRIS\n(UNLOCKED)"
                                  color:UIColor.greenColor
                                   size:card
                                 action:^{
        gGameSettings.calibrationRequested = true;
        [weakSelf setState:GameState::PLAYING];
    }];
    UIView *level1 = [self buttonTitled:@"MISSION 1:\nLUNAR BASE\n(LOCKED 🔒)"
                                  color:UIColor.grayColor size:card action:nil];
    UIView *level2 = [self buttonTitled:@"MISSION 2:\nAD CAELOS\n(LOCKED 🔒)"
                                  color:UIColor.grayColor size:card action:nil];
    UIView *back = [self buttonTitled:@"⬅️ BACK TO MAIN MENU"
                                color:UIColor.whiteColor
                                 size:CGSizeMake(260, 48)
                               action:^{
        [weakSelf setState:GameState::MAIN_MENU];
    }];

    UIView *content = makeStack(UILayoutConstraintAxisVertical, 16, @[
        makeLabel(@"SELECT MISSION", UIColor.cyanColor, 22, UIFontWeightBold),
        makeStack(UILayoutConstraintAxisHorizontal, 12, @[level0, level1, level2]),
        back,
    ]);
    return [self addScreen:content background:rgba(5, 10, 20, 180)];
}

- (UIView *)makeWorkshop {
    __weak SFMenus *weakSelf = self;
    UILabel *stats = makeLabel(
            @"--- SHIP STATS & CONFIG ---\n"
            "HULL SHIELDS   : 100 HP [MAX]\n"
            "THRUST POWER   : 6.0 u/s\n"
            "TURNING AGILITY: 100%\n"
            "STATUS         : COMBAT READY",
            UIColor.greenColor, 12, UIFontWeightRegular);
    stats.textAlignment = NSTextAlignmentLeft;
    stats.backgroundColor = rgba(10, 20, 35, 160);

    UIView *back = [self buttonTitled:@"⬅️ BACK TO MAIN MENU"
                                color:UIColor.whiteColor
                                 size:CGSizeMake(190, 72)
                               action:^{
        [weakSelf setState:GameState::MAIN_MENU];
    }];

    UIView *content = makeStack(UILayoutConstraintAxisVertical, 16, @[
        makeLabel(@"HANGAR & SHIP WORKSHOP", UIColor.cyanColor, 22, UIFontWeightBold),
        makeStack(UILayoutConstraintAxisHorizontal, 24, @[stats, back]),
    ]);
    return [self addScreen:content background:rgba(5, 12, 25, 190)];
}

- (UIView *)makePause {
    __weak SFMenus *weakSelf = self;
    CGSize size = CGSizeMake(200, 52);
    UIView *grid = [self gridOf:@[
        [self buttonTitled:@"▶️ RESUME GAME" color:UIColor.greenColor size:size action:^{
            [weakSelf setState:GameState::PLAYING];
        }],
        [self buttonTitled:@"🎯 CALIBRATE" color:UIColor.cyanColor size:size action:^{
            gGameSettings.calibrationRequested = true;
        }],
        [self buttonTitled:@"⚙️ SETTINGS" color:UIColor.whiteColor size:size action:^{
            [weakSelf showSettings];
        }],
        [self buttonTitled:@"🏠 QUIT TO MAIN MENU" color:rgba(255, 100, 100) size:size action:^{
            [weakSelf setState:GameState::MAIN_MENU];
        }],
    ]];

    UIView *content = makeStack(UILayoutConstraintAxisVertical, 16, @[
        makeLabel(@"GAME PAUSED", UIColor.yellowColor, 24, UIFontWeightBold),
        grid,
    ]);
    return [self addScreen:content background:rgba(5, 10, 20, 180)];
}

- (UIView *)makeHud {
    __weak SFMenus *weakSelf = self;
    SFPassthroughView *hud = [[SFPassthroughView alloc] initWithFrame:_root.bounds];
    hud.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    UIView *pause = [self buttonTitled:@"⏸️ PAUSE"
                                 color:UIColor.whiteColor
                                  size:CGSizeMake(110, 44)
                                action:^{
        [weakSelf setState:GameState::PAUSED];
    }];
    UIView *calibrate = [self buttonTitled:@"🎯 CALIBRATE"
                                     color:UIColor.cyanColor
                                      size:CGSizeMake(130, 44)
                                    action:^{
        gGameSettings.calibrationRequested = true;
    }];
    pause.translatesAutoresizingMaskIntoConstraints = NO;
    calibrate.translatesAutoresizingMaskIntoConstraints = NO;
    [hud addSubview:pause];
    [hud addSubview:calibrate];

    UILayoutGuide *safe = hud.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [pause.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:16],
        [pause.topAnchor constraintEqualToAnchor:safe.topAnchor constant:12],
        [calibrate.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-16],
        [calibrate.topAnchor constraintEqualToAnchor:safe.topAnchor constant:12],
    ]];

    [_root addSubview:hud];
    return hud;
}

- (UIView *)sliderTitled:(NSString * (^)(float))title
                     min:(float)min
                     max:(float)max
                   value:(float)value
                onChange:(void (^)(float))onChange {
    UILabel *label = makeLabel(title(value), UIColor.whiteColor, 13, UIFontWeightRegular);
    UISlider *slider = [[UISlider alloc] init];
    slider.minimumValue = min;
    slider.maximumValue = max;
    slider.value = value;
    [slider.widthAnchor constraintEqualToConstant:320].active = YES;

    __weak UISlider *weakSlider = slider;
    __weak UILabel *weakLabel = label;
    [slider addAction:[UIAction actionWithHandler:^(__kindof UIAction *sender) {
        float current = weakSlider.value;
        weakLabel.text = title(current);
        onChange(current);
    }]
        forControlEvents:UIControlEventValueChanged];

    return makeStack(UILayoutConstraintAxisVertical, 2, @[label, slider]);
}

- (void)showSettings {
    UIView *music = [self sliderTitled:^NSString *(float v) {
        return [NSString stringWithFormat:@"Music Volume: %d%%", (int) (v * 100)];
    } min:0 max:1 value:gGameSettings.musicVolume onChange:^(float v) {
        gGameSettings.musicVolume = v;
    }];
    UIView *sfx = [self sliderTitled:^NSString *(float v) {
        return [NSString stringWithFormat:@"SFX Volume: %d%%", (int) (v * 100)];
    } min:0 max:1 value:gGameSettings.sfxVolume onChange:^(float v) {
        gGameSettings.sfxVolume = v;
    }];
    UIView *sensitivity = [self sliderTitled:^NSString *(float v) {
        return [NSString stringWithFormat:@"Steering Sensitivity: %.1fx", v];
    } min:1 max:11 value:gGameSettings.turnSensitivity onChange:^(float v) {
        gGameSettings.turnSensitivity = v;
    }];

    __block __weak UIView *weakScreen = nil;
    UIView *close = [self buttonTitled:@"SAVE & CLOSE"
                                 color:UIColor.cyanColor
                                  size:CGSizeMake(180, 44)
                                action:^{
        [weakScreen removeFromSuperview];
    }];

    UIView *content = makeStack(UILayoutConstraintAxisVertical, 10, @[
        makeLabel(@"AUDIO & CONTROL SETTINGS", UIColor.cyanColor, 16, UIFontWeightBold),
        music, sfx, sensitivity, close,
    ]);
    UIView *screen = [self addScreen:content background:rgba(5, 10, 20, 235)];
    weakScreen = screen;
}
@end

static SFMenus *sMenus;
static SFMotion *sMotion;

void iosPlatformInstall(SDL_Window *window) {
    UIWindow *uiWindow = (__bridge UIWindow *) SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
    UIView *host = uiWindow.rootViewController.view;

    sMenus = [[SFMenus alloc] initWithHost:host];
    sMotion = [[SFMotion alloc] init];
    sMotion.view = host;
    [sMotion start];
}

void iosPlatformSetActive(bool active) {
    if (active) {
        [sMotion start];
    } else {
        [sMotion stop];
    }
}
