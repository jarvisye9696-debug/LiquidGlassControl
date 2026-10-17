// SPDX-License-Identifier: GPL-3.0-only
// LiquidGlass material rules extracted from m417z's Start Styler.
// Layout/size/text/visibility and visual-state assignments are excluded.
inline const Theme Start = {{
    ThemeTargetStyles{L"Border#AcrylicBorder", {
        L"Background:=$Background",
        L"BorderBrush:=$BorderBrush",
        L"BorderThickness=$BorderThickness",
        L"CornerRadius=$CornerRadius"}},
    ThemeTargetStyles{L"Border#AppBorder", {
        L"Background:=$Background",
        L"BorderBrush:=$BorderBrush",
        L"BorderThickness=$BorderThickness",
        L"CornerRadius=$CornerRadius"}},
    ThemeTargetStyles{L"Border#ContentBorder@CommonStates > Grid > Border#BackgroundBorder", {
        L"BorderThickness=$ElementBorderThickness",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"Button > Grid@CommonStates > Border#BackgroundBorder", {
        L"BorderThickness=$ElementBorderThickness",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"StartMenu.CategoryControl > Grid > Border", {
        L"BorderThickness=$ElementBorderThickness",
        L"BorderBrush:=$ElementBorderBrush",
        L"Background:=$ElementBackground",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"Button#Header > Border@CommonStates", {
        L"BorderThickness=$ElementBorderThickness",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"ListViewItem > Grid@CommonStates > Border#BorderBackground", {
        L"BorderThickness=$ElementBorderThickness",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"StartMenu.SearchBoxToggleButton > Grid@CommonStates > Border#BorderElement", {
        L"CornerRadius=$ElementCornerRadius",
        L"BorderThickness=$ElementBorderThickness",
        L"BorderBrush:=$ElementBorderBrush"}},
    ThemeTargetStyles{L"Button#HideMoreSuggestionsButton > Grid@CommonStates > Border#BackgroundBorder", {
        L"BorderThickness=$ElementBorderThickness",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"Button#ShowMoreSuggestionsButton > Grid@CommonStates > Border#BackgroundBorder", {
        L"BorderThickness=$ElementBorderThickness",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"StartMenu.FolderModal > Grid#Root > Border", {
        L"BorderThickness=$BorderThickness",
        L"BorderBrush:=$BorderBrush",
        L"CornerRadius=$CornerRadius"}},
    ThemeTargetStyles{L"MenuFlyoutPresenter > Border", {
        L"BorderThickness=$BorderThickness",
        L"BorderBrush:=$BorderBrush",
        L"CornerRadius=$CornerRadius"}},
    ThemeTargetStyles{L"Border#ContentBorder@CommonStates > Grid#DroppedFlickerWorkaroundWrapper > ContentPresenter#ContentPresenter > ContentControl > Grid#RootGrid > Border#LogoBackgroundPlate > Image#AllAppsItemLogo", {
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"StartDocked.NavigationPaneButton > Grid@CommonStates > Border#BackgroundBorder", {
        L"BorderThickness=$ElementBorderThickness",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"StartDocked.AppListViewItem > Grid@CommonStates > Border#BackgroundBorder", {
        L"BorderThickness=$BorderThickness",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"Microsoft.UI.Xaml.Controls.DropDownButton > Grid@CommonStates", {
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"Border#ContentBorder@CommonStates > Grid#DroppedFlickerWorkaroundWrapper > ContentPresenter#ContentPresenter > ContentControl > Grid#RootGrid > Grid#LogoContainer > Image#AllAppsTileLogo", {
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"Border#ContentBorder@CommonStates > Grid#DroppedFlickerWorkaroundWrapper > ContentPresenter > Grid > Grid#LogoContainer > Grid", {
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"Grid#ContentBorder@CommonStates > Grid#DroppedFlickerWorkaroundWrapper > ContentPresenter > Grid > Grid#LogoContainer > Grid", {
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"FlyoutPresenter > Border > ScrollViewer > Border > Grid > ScrollContentPresenter > ContentPresenter > Border", {
        L"BorderBrush:=$BorderBrush",
        L"BorderThickness=$BorderThickness",
        L"CornerRadius=$CornerRadius"}},
    ThemeTargetStyles{L"Button > ContentPresenter#ContentPresenter@CommonStates", {
        L"BorderThickness=$ElementBorderThickness",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"Grid@SearchBoxInputStates > Border#TaskbarSearchBackground", {
        L"CornerRadius=$ElementCornerRadius",
        L"BorderBrush:=$ElementBorderBrush",
        L"BorderThickness=$ElementBorderThickness"}},
}, {

    L"BorderThickness=0.3,1,0.3,0.3",
    L"ElementBorderThickness=0.3,0.3,0.3,1",
    L"CornerRadius=12",
    L"ElementCornerRadius=8",
    L"Background=<WindhawkBlur BlurAmount=\"15\" TintColor=\"{ThemeResource SystemAltLowColor}\" TintOpacity=\"0.2\" />",
    L"ElementBackground=<WindhawkBlur BlurAmount=\"15\" TintColor=\"{ThemeResource SystemAltLowColor}\" TintOpacity=\"0.4\" />",
    L"ElementBackground2 = <WindhawkBlur BlurAmount=\"15\" TintColor=\"{ThemeResource SystemAltLowColor}\" TintOpacity=\"0.2\"  />",
    L"AccentBackground=<WindhawkBlur BlurAmount=\"15\" TintColor=\"{ThemeResource SystemAccentColorLight1}\" TintOpacity=\"0.2\"  />",
    L"BorderBrush=<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#50808080\" Offset=\"0.0\" /><GradientStop Color=\"#50404040\" Offset=\"0.25\" /><GradientStop Color=\"#50808080\" Offset=\"1\" /></LinearGradientBrush>\"",
    L"ElementBorderBrush=<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#50808080\" Offset=\"1\" /><GradientStop Color=\"#50606060\" Offset=\"0.15\" /></LinearGradientBrush>",
}};
