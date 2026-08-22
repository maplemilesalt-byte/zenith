#include "cpu/cpu.h"
#include "elf_loader.h"
#include "memory/guest_memory.h"
#include "renderer.h"
#include "window.h"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

int main(int argc,char**argv){
 constexpr int width=1280,height=720;
 std::cout<<"Zenith 0.3.0\nInitializing guest memory and x86-64 CPU...\n";
 GuestMemory memory(64ull*1024ull*1024ull);
 const bool elfMode=argc>=3&&(std::string(argv[1])=="--elf"||std::string(argv[1])=="--graphics");
 const bool graphicsMode=argc>=3&&std::string(argv[1])=="--graphics";
 constexpr std::uint64_t fb=0x800000,fbMapSize=0x50000;
 if(elfMode){
  if(graphicsMode&&!memory.map(fb,fb,fbMapSize,GuestMemory::Permissions::Read|GuestMemory::Permissions::Write)){std::cerr<<"Failed to map guest framebuffer.\n";return 1;}
  ElfLoader loader;ElfLoadResult result;std::string error;
  if(!loader.loadFile(argv[2],memory,result,error)){std::cerr<<"ELF load error: "<<error<<'\n';return 1;}
  std::cout<<"ELF loaded: entry = 0x"<<std::hex<<result.entryPoint<<", program headers = "<<std::dec<<result.programHeaders<<'\n';
  CPU cpu(memory);cpu.setRip(result.entryPoint);constexpr std::uint64_t maxInstructions=1'000'000;std::uint64_t executed=0;
  while(!cpu.halted()&&executed<maxInstructions){if(!cpu.step()){std::cerr<<"CPU error after ELF load at RIP 0x"<<std::hex<<cpu.rip()<<": "<<cpu.lastError()<<'\n';return 1;}++executed;}
  if(!cpu.halted()){std::cerr<<"ELF execution stopped after "<<std::dec<<maxInstructions<<" instructions (possible infinite loop).\n";return 1;}
  if(graphicsMode&&cpu.framePresented()){
   std::cout<<"Guest presented framebuffer 320x240 after "<<std::dec<<executed<<" instructions.\n";
   XenithWindow window(width,height,"Zenith - Xbox Series X|S Emulator");Renderer renderer(width,height);
   while(window.isOpen()){window.pollEvents();renderer.clear(0x00101018);if(!renderer.drawGuestFramebuffer(memory,fb,320,240)){std::cerr<<"Failed to read guest framebuffer.\n";return 1;}window.present(renderer.pixels(),renderer.width(),renderer.height());std::this_thread::sleep_for(std::chrono::milliseconds(16));}
   return 0;
  }
  std::cout<<"\nELF exited with code "<<std::dec<<cpu.exitCode()<<" after "<<executed<<" instructions.\n";return static_cast<int>(cpu.exitCode());
 }
 constexpr std::uint64_t codeAddress=0x1000;auto permissions=GuestMemory::Permissions::Read|GuestMemory::Permissions::Write|GuestMemory::Permissions::Execute;
 if(!memory.map(codeAddress,0,GuestMemory::PageSize,permissions)){std::cerr<<"Failed to map guest code page.\n";return 1;}
 const std::uint8_t program[]={0xB8,0x2A,0,0,0,0xB9,0x08,0,0,0,0x48,0x01,0xC8};
 for(std::size_t i=0;i<sizeof(program);++i)if(!memory.write8(codeAddress+i,program[i])){std::cerr<<"Failed to load guest program.\n";return 1;}
 CPU cpu(memory);cpu.setRip(codeAddress);for(int i=0;i<3;++i)if(!cpu.step()){std::cerr<<"CPU error: "<<cpu.lastError()<<'\n';return 1;}
 std::cout<<"CPU self-test: RAX = "<<cpu.getRegister(CPU::RAX)<<'\n';
 XenithWindow window(width,height,"Zenith - Xbox Series X|S Emulator");Renderer renderer(width,height);float angle=0;
 while(window.isOpen()){window.pollEvents();renderer.clear(0x00101018);renderer.drawCube(angle);window.present(renderer.pixels(),renderer.width(),renderer.height());angle+=0.01f;std::this_thread::sleep_for(std::chrono::milliseconds(16));}
 std::cout<<"Zenith shutting down.\n";return 0;
}
