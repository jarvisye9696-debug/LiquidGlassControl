// SPDX-License-Identifier: GPL-3.0-only
// LiquidGlass material rules extracted from m417z's Notifications Styler.
// Layout/size/text/visibility and visual-state assignments are excluded.
inline const Theme Notifications = {{
    ThemeTargetStyles{L"Grid#NotificationCenterGrid", {
        L"Background:=$Background",
        L"CornerRadius = $CornerRadius",
        L"BorderThickness = $BorderThickness",
        L"BorderBrush := $BorderBrush"}},
    ThemeTargetStyles{L"Grid#CalendarCenterGrid", {
        L"Background:=$Background",
        L"CornerRadius=$CornerRadius",
        L"BorderThickness = $BorderThickness",
        L"BorderBrush := $BorderBrush"}},
    ThemeTargetStyles{L"ScrollViewer#CalendarControlScrollViewer", {
        L"Background := $ElementBackground",
        L"CornerRadius = $ElementCornerRadius",
        L"BorderThickness = $ElementBorderThickness",
        L"BorderBrush := $ElementBorderBrush"}},
    ThemeTargetStyles{L"Border#CalendarHeaderMinimizedOverlay", {
        L"Background := $ElementBackground",
        L"CornerRadius = $CornerRadius",
        L"BorderThickness = $ElementBorderThickness",
        L"BorderBrush := $ElementBorderBrush"}},
    ThemeTargetStyles{L"ActionCenter.FocusSessionControl#FocusSessionControl > Grid#FocusGrid", {
        L"Background := $ElementBackground",
        L"CornerRadius=$CornerRadius",
        L"BorderThickness = $ElementBorderThickness",
        L"BorderBrush := $ElementBorderBrush"}},
    ThemeTargetStyles{L"MenuFlyoutPresenter", {
        L"Background := $Background",
        L"BorderThickness = 0,0,0,0",
        L"CornerRadius = $CornerRadius",
        L"BorderThickness = $BorderThickness",
        L"BorderBrush := $BorderBrush"}},
    ThemeTargetStyles{L"Border#JumpListRestyledAcrylic", {
        L"Background := $Background",
        L"BorderThickness = 0,0,0,0",
        L"CornerRadius = $CornerRadius",
        L"BorderThickness = $BorderThickness",
        L"BorderBrush := $BorderBrush"}},
    ThemeTargetStyles{L"Grid#ControlCenterRegion", {
        L"Background := $Background",
        L"CornerRadius = $CornerRadius",
        L"BorderThickness = 0,0,0,0"}},
    ThemeTargetStyles{L"ContentPresenter#PageContent", {
        L"Background := Transparent"}},
    ThemeTargetStyles{L"ContentPresenter#PageContent > Grid > Border", {
        L"Background := $ElementBackground",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"QuickActions.ControlCenter.AccessibleWindow#PageWindow > ContentPresenter > Grid#FullScreenPageRoot", {
        L"Background := Transparent"}},
    ThemeTargetStyles{L"QuickActions.ControlCenter.AccessibleWindow#PageWindow > ContentPresenter > Grid#FullScreenPageRoot > ContentPresenter#PageHeader", {
        L"Background := $ElementBackground",
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"ScrollViewer#ListContent", {
        L"Background := $ElementBackground",
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"ActionCenter.FlexibleToastView#FlexibleNormalToastView", {
        L"Background := Transparent"}},
    ThemeTargetStyles{L"Border#ToastBackgroundBorder, Border#ToastBackgroundBorder2", {
        L"Background :=$Background",
        L"BorderThickness = 0,0,0,0",
        L"CornerRadius = $CornerRadius"}},
    ThemeTargetStyles{L"JumpViewUI.SystemItemListViewItem > Grid#LayoutRoot > Border#BackgroundBorder", {
        L"Background := $ElementBackground",
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"JumpViewUI.JumpListListViewItem > Grid#LayoutRoot > Border#BackgroundBorder", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"ActionCenter.FlexibleItemView", {
        L"CornerRadius = $CornerRadius"}},
    ThemeTargetStyles{L"QuickActions.AccessibleToggleButton#ToggleButton", {
        L"CornerRadius = $ElementCornerRadius",
        L"BorderThickness = 0"}},
    ThemeTargetStyles{L"QuickActions.AccessibleToggleButton#SplitL2Button", {
        L"CornerRadius = $ElementCornerRadius",
        L"BorderThickness = 0"}},
    ThemeTargetStyles{L"Grid#NotificationCenterTopBanner", {
        L"Background := $ElementBackground",
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Grid#L1Grid > Border", {
        L"Background := Transparent"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.ContentPresenter", {
        L"BorderThickness = 0"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Button#FooterButton[AutomationProperties.Name = Edit quick settings]", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Button[AutomationProperties.AutomationId = Microsoft.QuickAction.Battery]", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Button#FooterButton[AutomationProperties.Name = All settings]", {
        L"CornerRadius = $ElementCornerRadius",
        L"Background := $Background"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Button[AutomationProperties.AutomationId = Microsoft.QuickAction.Volume]", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Button#VolumeL2Button[AutomationProperties.Name = Select a sound output]", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Shapes.Rectangle#HorizontalTrackRect", {
        L"Fill := $ElementBackground"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Grid#MediaTransportControlsRegion", {
        L"CornerRadius = $CornerRadius",
        L"BorderThickness = 0",
        L"Background := $Background"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Grid#ThumbnailImage", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Primitives.RepeatButton#PreviousButton > Windows.UI.Xaml.Controls.ContentPresenter#ContentPresenter@CommonStates", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Button#PlayPauseButton > Windows.UI.Xaml.Controls.ContentPresenter#ContentPresenter@CommonStates", {
        L"CornerRadius = $ElementCornerRadius",
        L"BorderBrush := $ElementBorderBrush",
        L"BorderThickness = $ElementBorderThickness"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Primitives.RepeatButton#NextButton > Windows.UI.Xaml.Controls.ContentPresenter#ContentPresenter@CommonStates", {
        L"CornerRadius = $ElementCornerRadius",
        L"BorderBrush := $ElementBorderBrush",
        L"BorderThickness = $ElementBorderThickness"}},
    ThemeTargetStyles{L"Grid#MediaTransportControlsRoot", {
        L"Background := Transparent"}},
    ThemeTargetStyles{L"Grid#ToastPeekRegion", {
        L"Background ="}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.CalendarViewDayItem > Windows.UI.Xaml.Controls.Border", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.CalendarViewDayItem", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Control > Windows.UI.Xaml.Controls.Border", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Primitives.CalendarViewItem", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.ListViewHeaderItem", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Button#SettingsButton", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Button#DismissButton", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Grid#WeekDayNames", {
        L"Background := $AccentBackground",
        L"CornerRadius = $ElementCornerRadius",
        L"BorderThickness = $ElementBorderThickness",
        L"BorderBrush := $ElementBorderBrush"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.ListViewItem", {
        L"CornerRadius = $ElementCornerRadius"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Grid#RootGrid > Windows.UI.Xaml.Controls.ContentPresenter#ContentPresenter", {
        L"Background := $AccentBackground",
        L"CornerRadius = $ElementCornerRadius",
        L"BorderThickness = $ElementBorderThickness",
        L"BorderBrush := $ElementBorderBrush"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Controls.Border#ItemOpaquePlating", {
        L"Background := $ElementBackground2",
        L"CornerRadius = $ElementCornerRadius",
        L"BorderThickness = $ElementBorderThickness",
        L"BorderBrush := $ElementBorderBrush"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Grid#StandardHeroContainer", {
        L"CornerRadius = 0"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Button#BackButton", {
        L"CornerRadius = $ElementCornerRadius",
        L"BorderBrush := $ElementBorderBrush",
        L"BorderThickness = $ElementBorderThickness"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Border#SwitchKnobOn", {
        L"CornerRadius = 3"}},
    ThemeTargetStyles{L"ActionCenter.NotificationListViewItem", {
        L"BorderThickness = $ElementBorderThickness",
        L"BorderBrush := $ElementBorderBrush"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.Grid[AutomationProperties.LocalizedLandmarkType = Footer]", {
        L"BorderThickness = $ElementBorderThickness",
        L"BorderBrush := $ElementBorderBrush"}},
    ThemeTargetStyles{L"NetworkUX.View.SettingsListViewItem > Windows.UI.Xaml.Controls.Primitives.ListViewItemPresenter#Root", {
        L"CornerRadius = $CornerRadius",
        L"BorderThickness = $ElementBorderThickness",
        L"BorderBrush := $ElementBorderBrush"}},
    ThemeTargetStyles{L"Windows.UI.Xaml.Controls.ContentPresenter > Windows.UI.Xaml.Controls.Border", {
        L"BorderThickness = $ElementBorderThickness",
        L"BorderBrush := $ElementBorderBrush"}},
}, {

    L"transparent = Transparent",
    L"Background = <WindhawkBlur BlurAmount=\"15\" TintColor=\"{ThemeResource SystemAltLowColor}\" TintOpacity=\"0.4\"  />",
    L"ElementBackground = <WindhawkBlur BlurAmount=\"20\" TintColor=\"{ThemeResource SystemAltLowColor}\" TintOpacity=\"0.4\"  />",
    L"AccentBackground =<WindhawkBlur BlurAmount=\"20\" TintColor=\"{ThemeResource SystemAccentColorLight1}\" TintOpacity=\"0.2\"  />",
    L"ElementBackground2 = <WindhawkBlur BlurAmount=\"15\" TintColor=\"{ThemeResource SystemAltLowColor}\" TintOpacity=\"0.2\"  />",
    L"BorderBrush=<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#50808080\" Offset=\"0.0\" /><GradientStop Color=\"#50404040\" Offset=\"0.25\" /><GradientStop Color=\"#50808080\" Offset=\"1\" /></LinearGradientBrush>",
    L"ElementBorderBrush=<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#50808080\" Offset=\"1\" /><GradientStop Color=\"#50606060\" Offset=\"0.15\" /></LinearGradientBrush>",
    L"BorderThickness=0.3,1,0.3,0.3",
    L"ElementBorderThickness=0.3,0.3,0.3,1",
    L"CornerRadius = 12",
    L"ElementCornerRadius = 8",
}};
