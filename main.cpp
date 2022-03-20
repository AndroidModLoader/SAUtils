#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <dlfcn.h>
#include <sautils.h>

/* Same name but can be used for VC too! (!!!not working currently!!!) */
MYMOD(net.rusjj.gtasa.utils, SAUtils, 1.2.1, RusJJ)
//NEEDGAME(com.rockstargames.gtasa)

uintptr_t pGameLib = 0;
void* pGameHandle = NULL;

void Redirect(uintptr_t addr, uintptr_t to)
{
    if(!addr) return;
    if(addr & 1)
    {
        addr &= ~1;
        if (addr & 2)
        {
            aml->PlaceNOP(addr, 1);
            addr += 2;
        }
        uint32_t hook[2];
        hook[0] = 0xF000F8DF;
        hook[1] = to;
        aml->Write(addr, (uintptr_t)hook, sizeof(hook));
    }
    else
    {
        uint32_t hook[2];
        hook[0] = 0xE51FF004;
        hook[1] = to;
        aml->Write(addr, (uintptr_t)hook, sizeof(hook));
    }
}

extern "C" void OnModPreLoad() // PreLoad is a place for interfaces registering
{
    logger->SetTag("SAUtils");
    pGameLib = aml->GetLib("libGTASA.so");
    pGameHandle = dlopen("libGTASA.so", RTLD_LAZY);
    ((SAUtils*)sautils)->m_pHasFLA = aml->GetLib("libplugin_fastman92limitAdjuster_ANDROID_ARM32.so");
    if(pGameLib && pGameHandle)
    {
        ((SAUtils*)sautils)->m_eLoadedGame = GTASA_2_00;
        ((SAUtils*)sautils)->InitializeSAUtils();
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
            logger->Error("Cannot determine the working game or this one is not supported!");
            return;
        }
    }
    
    RegisterInterface("SAUtils", sautils);
}