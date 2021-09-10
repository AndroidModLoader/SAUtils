#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <sautils.h>

/* Same name but can be used for VC too! (!!!not working currently!!!) */
MYMODCFG(net.rusjj.gtasa.utils, GTA SAUtils, 1.1, RusJJ)
NEEDGAME(com.rockstargames.gtasa)

uintptr_t pGameLib = 0;

extern "C" void OnModPreLoad() // PreLoad is a place for interfaces registering
{
    logger->SetTag("SAUtils");
    pGameLib = aml->GetLib("libGTASA.so");
    if(pGameLib)
    {
        ((SAUtils*)sautils)->m_eLoadedGame = GTASA_2_00;
        ((SAUtils*)sautils)->InitializeSAUtils();
    }
    else
    {
        pGameLib = aml->GetLib("libGTAVC.so");
        if(pGameLib)
        {
            ((SAUtils*)sautils)->m_eLoadedGame = GTAVC_1_09;
            ((SAUtils*)sautils)->InitializeVCUtils();
        }
    }

    ((SAUtils*)sautils)->m_pHasFLA = aml->GetLib("libplugin_fastman92limitAdjuster_ANDROID_ARM32.so");
    
    RegisterInterface("SAUtils", sautils);
}