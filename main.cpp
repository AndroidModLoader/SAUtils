#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <dlfcn.h>
#include <sautils.h>
#include <sautils_2_10.h>

/* Same name but can be used for VC too! */
MYMOD(net.rusjj.gtasa.utils, SAUtils, 1.4.1, RusJJ)

uintptr_t pGameLib = 0;
void* pGameHandle = NULL;


extern "C" void OnModPreLoad() // PreLoad is a place for interfaces registering
{
    logger->SetTag("SAUtils");
    pGameLib = aml->GetLib("libGTASA.so");
    pGameHandle = aml->GetLibHandle("libGTASA.so");
    if(pGameLib && pGameHandle)
    {
        if(*(uint32_t*)(pGameLib + 0x202020) == 0xE8BDB001)
        {
            ((SAUtils*)sautils)->m_eLoadedGame = GTASA_2_00;
            ((SAUtils*)sautils)->InitializeSAUtils();
        }
        else if(*(uint32_t*)(pGameLib + 0x202020) == 0x61766E49)
        {
            ((SAUtils_2_10*)sautils)->m_eLoadedGame = GTASA_2_10;
            ((SAUtils_2_10*)sautils)->InitializeSAUtils();
        }
    }
    else
    {
        //pGameLib = aml->GetLib("libGTAVC.so");
        //pGameHandle = dlopen("libGTAVC.so", RTLD_LAZY);
        //if(pGameLib && pGameHandle)
        //{
        //    ((SAUtils*)sautils)->m_eLoadedGame = GTAVC_1_09;
        //    ((SAUtils*)sautils)->InitializeVCUtils();
        //}
        //else
        {
            ((SAUtils*)sautils)->m_eLoadedGame = Unknown;
            logger->Error("Cannot determine the working game or this one is not supported!");
            return;
        }
    }
    
    ((SAUtils*)sautils)->m_pHasFLA = aml->GetLib("libplugin_fastman92limitAdjuster_ANDROID_ARM32.so");
    if(!((SAUtils*)sautils)->m_pHasFLA) ((SAUtils*)sautils)->m_pHasFLA = aml->GetLib("libplugin_fastman92limitAdjuster_ANDROID_ARM64.so");
    RegisterInterface("SAUtils", sautils);
}
