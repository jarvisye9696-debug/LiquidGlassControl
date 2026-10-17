// SPDX-License-Identifier: GPL-3.0-only
// LiquidGlass material rules extracted from m417z's Taskbar Styler.
// Layout/size/text/visibility and visual-state assignments are excluded.
inline const Theme Taskbar = {{
    ThemeTargetStyles{L"Taskbar.TaskbarFrame > Grid#RootGrid", {
        L"BorderThickness=$BorderThickness",
        L"BorderBrush:=$BorderBrush",
        L"Background:=$Background"}},
    ThemeTargetStyles{L"StackPanel#SystemTrayFrameGrid, Grid#SystemTrayFrameGrid", {
        L"Background:=$ElementBackground",
        L"BorderBrush:=$ElementBorderBrush",
        L"BorderThickness=$ElementBorderThickness",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"SystemTray.ChevronIconView", {
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"SystemTray.NotifyIconView#NotifyItemIcon", {
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"SystemTray.OmniButton", {
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"SystemTray.IconView#SystemTrayIcon > Grid#ContainerGrid > ContentPresenter#ContentPresenter > Grid#ContentGrid > SystemTray.TextIconContent > Grid#ContainerGrid", {
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"Border#OverflowFlyoutBackgroundBorder", {
        L"BorderThickness=$BorderThickness",
        L"BorderBrush:=$BorderBrush",
        L"Background:=$Background",
        L"CornerRadius=$CornerRadius"}},
    ThemeTargetStyles{L"WindowsInternal.ComposableShell.Experiences.Switcher.AltTab > Windows.UI.Xaml.Controls.Grid#ModalRootGrid > Windows.UI.Xaml.Controls.Border#BackgroundElement", {
        L"BorderThickness=0",
        L"Background=Transparent",
        L"CornerRadius=$CornerRadius"}},
    ThemeTargetStyles{L"WindowsInternal.ComposableShell.Experiences.Switcher.AltTab > Windows.UI.Xaml.Controls.Grid#ModalRootGrid > Windows.UI.Xaml.Controls.Border#BackgroundElement > WindowsInternal.ComposableShell.Experiences.Switcher.SwitchItemList", {
        L"Background:=$Background",
        L"BorderThickness=$BorderThickness",
        L"BorderBrush:=$BorderBrush",
        L"CornerRadius=$CornerRadius"}},
    ThemeTargetStyles{L"WindowsInternal.ComposableShell.Experiences.Switcher.VirtualDesktopBarElement#VirtualDesktopBar", {
        L"CornerRadius=$CornerRadius",
        L"Background:=$Background"}},
    ThemeTargetStyles{L"Border#BackgroundDimmingLayer", {
        L"Background:=$Background",
        L"CornerRadius=$CornerRadius"}},
    ThemeTargetStyles{L"Taskbar.TaskListButtonPanel#ExperienceToggleButtonRootPanel > Grid > Border#BackgroundElement, Taskbar.TaskListButtonPanel#ExperienceToggleButtonRootPanel > Border#BackgroundElement", {
        L"CornerRadius=$CornerRadius",
        L"BorderThickness=$ElementBorderThickness"}},
    ThemeTargetStyles{L"Taskbar.TaskListButton#TaskListButton", {
        L"CornerRadius=$ElementCornerRadius",
        L"BorderThickness=$ElementBorderThickness"}},
    ThemeTargetStyles{L"Border#SnapBarBorder", {
        L"Background:=$Background",
        L"BorderBrush:=$BorderBrush",
        L"CornerRadius=$CornerRadius",
        L"BorderThickness=$BorderThickness"}},
    ThemeTargetStyles{L"Grid#IconPanel@CommonStates > Border#BackgroundElement, Taskbar.TaskListLabeledButtonPanel@CommonStates > Border#BackgroundElement", {
        L"CornerRadius=$ElementCornerRadius",
        L"BorderThickness=$ElementBorderThickness"}},
    ThemeTargetStyles{L"ContentPresenter#ContentPresenter@CommonStates", {
        L"CornerRadius=$ElementCornerRadius",
        L"BorderThickness=$ElementBorderThickness"}},
    ThemeTargetStyles{L"Border#SnapPickerBorder", {
        L"Background:=$Background",
        L"BorderBrush:=$BorderBrush",
        L"CornerRadius=$CornerRadius",
        L"BorderThickness=$BorderThickness"}},
    ThemeTargetStyles{L"Taskbar.TaskListButtonPanel#ExperienceToggleButtonRootPanel", {
        L"Background:=Transparent"}},
    ThemeTargetStyles{L"ToolTip > ContentPresenter#LayoutRoot", {
        L"Background:=$Background",
        L"BorderBrush:=$BorderBrush",
        L"BorderThickness:=$BorderThickness",
        L"CornerRadius=$CornerRadius"}},
    ThemeTargetStyles{L"WindowsInternal.ComposableShell.Experiences.Switcher.VirtualDesktopBarElement > Grid#GridElement > Border#VirtualDesktopSwitcherBackground", {
        L"BorderBrush:=$BorderBrush",
        L"BorderThickness=$BorderThickness",
        L"CornerRadius=$CornerRadius",
        L"Background:=$Background"}},
    ThemeTargetStyles{L"WindowsInternal.ComposableShell.Experiences.Switcher.SwitchItemListViewItem > Grid > Border", {
        L"CornerRadius=$CornerRadius"}},
    ThemeTargetStyles{L"Border#VirtualDesktopBarBackground", {
        L"Background:=$Background",
        L"BorderBrush:=$BorderBrush",
        L"BorderThickness=$BorderThickness",
        L"CornerRadius=$CornerRadius"}},
    ThemeTargetStyles{L"Rectangle#RunningIndicator", {
        L"Fill:=$AccentBackground"}},
    ThemeTargetStyles{L"SearchUx.SearchUI.SearchIconButton > SearchUx.SearchUI.SearchButtonRootGrid@CommonStates > Grid > Border#BackgroundElement, SearchUx.SearchUI.SearchIconButton > SearchUx.SearchUI.SearchButtonRootGrid@CommonStates > Border#BackgroundElement", {
        L"Background:=Transparent",
        L"BorderBrush:=Transparent"}},
    ThemeTargetStyles{L"SearchUx.SearchUI.SearchButtonRootGrid", {
        L"Background:=Transparent",
        L"BorderBrush:=Transparent"}},
    ThemeTargetStyles{L"Border#SearchPillBackgroundElement", {
        L"BorderBrush:=$ElementBorderBrush",
        L"BorderThickness=$ElementBorderThickness",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"SearchUx.SearchUI.SearchBoxButton > SearchUx.SearchUI.SearchButtonRootGrid@CommonStates > Grid > Border#BackgroundElement, SearchUx.SearchUI.SearchBoxButton > SearchUx.SearchUI.SearchButtonRootGrid@CommonStates > Border#BackgroundElement", {
        L"CornerRadius=$ElementCornerRadius",
        L"BorderThickness=$ElementBorderThickness",
        L"BorderBrush:=$ElementBorderBrush",
        L"Background:=$ElementBackground"}},
    ThemeTargetStyles{L"Canvas#HoverFlyoutCanvas > Grid#HoverFlyoutGrid > Border#HoverFlyoutBackground", {
        L"Background:=$Background",
        L"BorderBrush:=$BorderBrush",
        L"BorderThickness=$BorderThickness",
        L"CornerRadius=$CornerRadius"}},
}, {

    L"Background=<WindhawkBlur BlurAmount=\"15\" TintColor=\"{ThemeResource SystemAltLowColor}\" TintOpacity=\"0.2\" />",
    L"ElementBackground=<WindhawkBlur BlurAmount=\"20\" TintColor=\"{ThemeResource SystemAltLowColor}\" TintOpacity=\"0.4\" />",
    L"ElementBackground2=<WindhawkBlur BlurAmount=\"20\" TintColor=\"{ThemeResource SystemAltLowColor}\" TintOpacity=\"0.2\" />",
    L"AccentBackground=<WindhawkBlur BlurAmount=\"15\" TintColor=\"{ThemeResource SystemAccentColorLight1}\" TintOpacity=\"0.2\" />",
    L"BorderBrush=<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#50808080\" Offset=\"0.0\" /><GradientStop Color=\"#50404040\" Offset=\"0.25\" /><GradientStop Color=\"#50808080\" Offset=\"1\" /></LinearGradientBrush>",
    L"ElementBorderBrush=<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#50808080\" Offset=\"1\" /><GradientStop Color=\"#50606060\" Offset=\"0.15\" /></LinearGradientBrush>",
    L"BorderThickness=0.3,1,0.3,0.3",
    L"ElementBorderThickness=0.3,0.3,0.3,1",
    L"CornerRadius=12",
    L"ElementCornerRadius=8",
}};
