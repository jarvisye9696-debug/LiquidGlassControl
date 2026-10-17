// SPDX-License-Identifier: GPL-3.0-only
// LiquidGlass material rules extracted from m417z's Explorer Styler.
// Layout/size/text/visibility and visual-state assignments are excluded.
inline const Theme Explorer = {{
    ThemeTargetStyles{L"Microsoft.UI.Xaml.Controls.Grid#PART_LayoutRoot", {
        L"Background=Transparent"}},
    ThemeTargetStyles{L"Microsoft.UI.Xaml.Controls.Primitives.NavigationViewItemPresenter#NavigationViewItemPresenter > Microsoft.UI.Xaml.Controls.Grid#LayoutRoot", {
        L"BorderThickness=$ElementBorderThickness",
        L"Background:=$ElementBackground",
        L"BorderBrush:=$ElementBorder",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"Microsoft.UI.Xaml.Controls.Grid#NavigationBarControlGrid", {
        L"Background:=Transparent",
        L"BorderBrush:=Transparent"}},
    ThemeTargetStyles{L"Microsoft.UI.Xaml.Controls.Grid#HomeViewRootGrid", {
        L"BorderBrush:=$ElementBorderBrush",
        L"CornerRadius=$ElementCornerRadius",
        L"BorderThickness=$ElementBorderThickness"}},
    ThemeTargetStyles{L"FileExplorerExtensions.GalleryViewControl#GalleryViewControl > Grid", {
        L"BorderBrush:=$ElementBorderBrush",
        L"CornerRadius=$ElementCornerRadius",
        L"BorderThickness=$ElementBorderThickness"}},
    ThemeTargetStyles{L"FileExplorerExtensions.GalleryViewControl#GalleryViewControl > Grid > Grid#GalleryRootGrid", {
        L"Background:=Transparent"}},
    ThemeTargetStyles{L"ToolTip", {
        L"BorderBrush:=$ElementBorderBrush",
        L"BorderThickness=$ElementBorderThickness",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"TabViewItem > Grid#LayoutRoot", {
        L"BorderThickness=$ElementBorderThickness",
        L"CornerRadius=$ElementCornerRadius",
        L"BorderBrush:=$ElementBorderBrush"}},
    ThemeTargetStyles{L"TabViewItem > Grid#LayoutRoot > Grid#TabContainer", {
        L"Background=Transparent",
        L"BorderBrush=Transparent"}},
    ThemeTargetStyles{L"CommandBarOverflowPresenter#SecondaryItemsControl > Grid#LayoutRoot", {
        L"Background:=$ElementBackground",
        L"BorderThickness=$ElementBorderThickness",
        L"BorderBrush:=$ElementBorderBrush",
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"Microsoft.UI.Xaml.Controls.AutoSuggestBox#FileExplorerSearchBox > Microsoft.UI.Xaml.Controls.Grid#LayoutRoot > Microsoft.UI.Xaml.Controls.TextBox#TextBox", {
        L"CornerRadius=$ElementCornerRadius",
        L"Background:=$ElementBackground",
        L"BorderBrush:=$ElementBorderBrush",
        L"BorderThickness=$ElementBorderThickness"}},
    ThemeTargetStyles{L"Microsoft.UI.Xaml.Controls.Grid#FileExplorerAddressBarGrid", {
        L"CornerRadius=$ElementCornerRadius",
        L"Background:=$ElementBackground",
        L"BorderBrush:=$ElementBorderBrush",
        L"BorderThickness=$ElementBorderThickness"}},
    ThemeTargetStyles{L"Microsoft.UI.Xaml.Controls.AutoSuggestBox#PART_AutoSuggestBox > Microsoft.UI.Xaml.Controls.Grid#LayoutRoot > Microsoft.UI.Xaml.Controls.TextBox#TextBox", {
        L"CornerRadius=$ElementCornerRadius"}},
    ThemeTargetStyles{L"Microsoft.UI.Xaml.Controls.Grid#RootContainer", {
        L"Background:=Transparent"}},
    ThemeTargetStyles{L"Grid#DetailsViewControlRootGrid", {
        L"Background:=Transparent"}},
    ThemeTargetStyles{L"StackPanel#DetailsViewThumbnail", {
        L"Background:=Transparent"}},
}, {

    L"ContentBG=<SolidColorBrush Color=\"{ThemeResource SystemChromeAltHighColor}\" Opacity=\"1\" />",
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
