.pragma library

// Tech Aim PDF report branding metrics (T3.1). ONE place for the logo scale and
// brand colours so every user-facing exported report shares the same
// (enlarged) Tech Aim wordmark. The approved light-background wordmark is
// images/logo/techaim_color.png (aspect ≈ 3.251:1). Header target: ~25% of the
// 794 px A4 page width → height 60 px → width ≈ 195 px.

var logoSource       = "qrc:/images/logo/techaim_color.png"
var logoSourceWhite  = "qrc:/images/logo/techaim_white.png"
var logoSourceBlack  = "qrc:/images/logo/techaim_black.png"
var logoAspect       = 3163 / 973          // width / height
var logoHeaderHeight = 60                    // main header logo (px on the A4 page)
var logoFooterHeight = 16                    // small footer mark
var minHeaderLogoWidthPx = 170               // acceptance floor for the header logo

// width for a given rendered height, aspect preserved (never stretched).
function logoWidth(h) { return h * logoAspect }

var accentRed = "#C40046"
var ink       = "#14171C"
var sub       = "#5C636E"
var line      = "#D5D9DF"

var tagline   = "WE AIM TO PLEASE"
var org       = "Tech Aim Electronic Target Control"
