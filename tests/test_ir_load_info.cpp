#include "IRLoadInfo.h"
#include <cassert>
#include <string>
int main(){
 using vm::IRError;
 assert(vm::validIRPath("C:/Users/test.wav"));
 assert(vm::validIRPath("C:/IR/\xC3\xBC.wav"));
 assert(!vm::validIRPath(""));
 assert(vm::validIRPath("",true)); // explicit CLEAR command
 assert(!vm::validIRPath(std::string(4097,'x')));
 assert(!vm::validIRPath(std::string("a")+'\0'+"b"));
 assert(!vm::validIRPath("bad\nname.wav"));
 assert(vm::identifyIRError("Cannot open IR file")==IRError::Missing);
 assert(vm::identifyIRError("IR file empty or too large")==IRError::TooLarge);
 assert(vm::identifyIRError("Not RIFF/WAVE")==IRError::Invalid);
 assert(vm::identifyIRError("Cannot prepare IR for host sample rate")==IRError::Prepare);
 assert(vm::identifyIRError("unknown")==IRError::Other);
 assert(std::string(vm::irErrorLabel(2))=="FILE TOO LARGE");
}
