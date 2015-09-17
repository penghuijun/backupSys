#include "GYinGetTarget.h"

ContentCategory GYin_AndroidgetTargetCat(string& cat)
{
    if(cat == "Games")
        return CAT_805;
    else if(cat == "Apps")
        return CAT_805;
    else if(cat == "Catalogues")
        return CAT_824;
    else if(cat == "Books&Reference")
        return CAT_82005;
    else if(cat == "Business")
        return CAT_819;
    else if(cat == "Comics")
        return CAT_80603;
    else if(cat == "Communication")
        return CAT_82202;
    else if(cat == "Education")
        return CAT_806;
    else if(cat == "Enterainment")
        return CAT_824;
    else if(cat == "Finance")
        return CAT_802;
    else if(cat == "Health&Fitness")
        return CAT_804;
    else if(cat == "Libraries&Demo")
        return CAT_820;
    else if(cat == "Communication")
        return CAT_82202;
    else if(cat == "Lifestyle")
        return CAT_818;
    else if(cat == "Live Wallpaper")
        return CAT_80604;
    else if(cat == "Media&Video")
        return CAT_80601;
    else if(cat == "Medical")
        return CAT_804;
    else if(cat == "Music&Audio")
        return CAT_806;
    else if(cat == "News&Magazines")
        return CAT_82208;
    else if(cat == "Personalization")
        return CAT_814;
    else if(cat == "Photography")
        return CAT_82001;
    else if(cat == "Productivity")
        return CAT_817;
    else if(cat == "Shopping")
        return CAT_82204;
    else if(cat == "Social")
        return CAT_823;
    else if(cat == "Sports")
        return CAT_812;
    else if(cat == "Tools")
        return CAT_82507;
    else if(cat == "Transportation")
        return CAT_81302;
    else if(cat == "Travel&Local")
        return CAT_813;
    else if(cat == "Weather")
        return CAT_81307;
    else if(cat == "Widgets")
        return CAT_82507;        
    else if(cat == "Action")
        return CAT_805;
    else if(cat == "Adventure")
        return CAT_805;
    else if(cat == "Arcade")
        return CAT_805;
    else if(cat == "Board")
        return CAT_805;
    else if(cat == "Card")
        return CAT_805;
    else if(cat == "Casino")
        return CAT_805;
    else if(cat == "Education")
        return CAT_805;
    else if(cat == "Family")
        return CAT_805;
    else if(cat == "Music")
        return CAT_805;
    else if(cat == "Puzzle")
        return CAT_805;
    else if(cat == "Racing")
        return CAT_805;
    else if(cat == "Role Playing")
        return CAT_805;
    else if(cat == "Simulation")
        return CAT_805;
    else if(cat == "Sports")
        return CAT_805;
    else if(cat == "Strategy")
        return CAT_805;
    else if(cat == "Trivia")
        return CAT_805;
    else if(cat == "Word")
        return CAT_805;
    else if(cat == "Others")
        return CAT_805;
    else if(cat == "UNKNOWN")
        return CAT_805;
    else
        return CAT_805;

}
ContentCategory GYin_IOSgetTargetCat(string& cat)
{
    if(cat == "Games")
        return CAT_805;
    else if(cat == "Kids")
        return CAT_803;
    else if(cat == "Education")
        return CAT_808;
    else if(cat == "Newsstand")
        return CAT_82204;
    else if(cat == "Photo&Video")
        return CAT_80601;
    else if(cat == "Productivity")
        return CAT_817;
    else if(cat == "Lifestyle")
        return CAT_818;
    else if(cat == "Health&Fitness")
        return CAT_804;
    else if(cat == "Travel")
        return CAT_813;
    else if(cat == "Music")
        return CAT_80602;
    else if(cat == "Sports")
        return CAT_812;
    else if(cat == "Business")
        return CAT_819;
    else if(cat == "News")
        return CAT_82208;
    else if(cat == "Utilities")
        return CAT_823;
    else if(cat == "Entertainment")
        return CAT_806;
    else if(cat == "Social Networking")
        return CAT_822;
    else if(cat == "Food&Drink")
        return CAT_815;
    else if(cat == "Finance")
        return CAT_802;
    else if(cat == "Reference")
        return CAT_825;
    else if(cat == "Navigation")
        return CAT_82207;
    else if(cat == "Medical")
        return CAT_804;
    else if(cat == "Books")
        return CAT_82005;
    else if(cat == "Weather")
        return CAT_81307;
    else if(cat == "Catalogues")
        return CAT_824;
    else if(cat == "UNKNOWN")
        return CAT_824;
    else
        return CAT_824;
}
ConnectionType GYin_getConnectionType(int id)
{
    switch(id)
    {
        case 0:
            return UNKNOWN_TYPE;
        case 1:
            return ETHERNET;
        case 2:
            return WIFI;
        case 3:
            return CELLULAR_NETWORK_UNKNOWN_GENERATION;
        case 4:
            return CELLULAR_NETWORK_2G;
        case 5:
            return CELLULAR_NETWORK_3G;
        case 6:
            return CELLULAR_NETWORK_4G;
        default:
            break;
    }
}

