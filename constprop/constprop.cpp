#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"

#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <regex>

using namespace llvm;
using namespace std;

namespace {

struct constprop : public ModulePass
{
  static char ID;

  constprop : ModulePass(ID) {} // Constructor initializing the ModulePass with ID.

  // Set to store functions encountered during analysis.
  std::set<Function*> FunSet;
  
  // Data structure to store data flow information for each instruction.
  map<Instruction*, string> InSet;

  // Data structures to track data flow within basic blocks.
  map<BasicBlock*, map<Instruction*, string>> Gen;
  map<BasicBlock*, map<Instruction*, string>> Kill;

  // Data structure to manage function argument values.
  map<string, map<int, string>> FunArgMap;

  // Function to initialize data flow values for instructions in a given module
  map<Instruction*, string> Start(Module &M)
  {
    int count = 0;

    // Map to store data flow values for instructions
    map<Instruction*, string> InSet;

    for (Function &F : M)
    {
      for (BasicBlock &BB : F)
      {
        for (Instruction &I : BB)
        {
          // Check if the instruction is a call instruction
          if (isa<CallInst>(&I))
          {
            // Set data flow value to "BOTTOM" for call instructions
            InSet[&I] = "BOTTOM";

            // Initialize Gen and Kill maps if count is 0
            if (count == 0)
            {
              Gen[&BB][&I] = "BOTTOM";
              Kill[&BB][&I] = "BOTTOM";
            }
          }
          else
          {
            // Set data flow value to "TOP" for non-call instructions
            InSet[&I] = "TOP";

            if (count == 0)
            {
              Gen[&BB][&I] = "TOP";
              Kill[&BB][&I] = "TOP";
            }
          }
        }
      }
    }
    // Increment init_count after initialization
    count++;

    return InSet;
  }

  // Function to perform meet operation
  map<Instruction*, string> DoMeet(map<Instruction*, string> Kill, map<Instruction*, string> Gen, BasicBlock* BB)
  {
    map<Instruction*, string> Map1;

    for (auto K : Kill)
    {
      Instruction *I = K.first;

      // Check if either the previous or current analysis result for this instruction is "BOTTOM"

      if (Kill[I] == "BOTTOM" || Gen[I] == "BOTTOM")
        Map1[I] = "BOTTOM";

      // Check if neither the previous nor current analysis result is "TOP"
      else if ((Kill[I] != "TOP" && Gen[I] != "TOP"))
      {
        // If the previous and current results are the same, set the result to that value
        if (Kill[I] == Gen[I])
          Map1[I] = Kill[I];
      
        // If the previous and current results differ, set the result to "BOTTOM"
        else
          Map1[I] = "BOTTOM";
      }
      // If one of the results is "TOP", set the result to the non "TOP" value
      else
        Map1[I] = (Kill[I] != "TOP") ? Kill[I] : Gen[I];
    }
    return Map1;
  }

  string Intersect(string str1, string str2)
  {
    if (str1 == "BOTTOM" || str2 == "BOTTOM")
    {
      return "BOTTOM"; // If either input is "BOTTOM", the result is "BOTTOM"
    }
    else if (str1 != "TOP" && str2 != "TOP")
    {
      return (str1 == str2) ? str1 : "BOTTOM";
    }
    else
    {
      if (str1 != "TOP")
        return str1;
      else
        return str2;
    }
  }

  // Function to check if two maps are equal
  bool MapCmp(map<Instruction*, string> Kill, map<Instruction*, string> Gen)
  {
    // Iterate through each key-value pair in the 'Kill' map
    for (auto &pair : Kill)
    {
      // Check if the key exists in 'Gen'
      auto itr = Gen.find(pair.first);
      if (itr != Gen.end())
      {
        // Compare the corresponding values for the current key
        if (pair.second != itr->second)
        {
          // Values are different, maps are not equal
          return false;
        }
      }
    }
    // If all keys and values are equal, maps are equal
    return true;
  }

  // Compute the values
  string Compute(Instruction* I, string str1, string str2)
  {
    // Check if either input is "BOTTOM"
    if (str1 == "BOTTOM" || str2 == "BOTTOM")
      return "BOTTOM";

    // Check if neither input is "TOP"
    else if (str1 != "TOP" && str2 != "TOP")
    {
      // Add Instruction
      if (I->getOpcode() == Instruction::Add)
        return to_string(stoi(str1) + stoi(str2)); // Perform addition

      // FP Add Instruction
      else if (I->getOpcode() == Instruction::FAdd)
        return to_string(stoi(str1) + stoi(str2)); // Perform floating point addition

      // Subtract Instruction
      else if (I->getOpcode() == Instruction::Sub)
        return to_string(stoi(str1) - stoi(str2)); // Perform subtraction

      // FP Subtract Instruction
      else if (I->getOpcode() == Instruction::FSub)
        return to_string(stoi(str1) - stoi(str2)); // Perform floating point subtraction

      // Multiply Instruction
      else if (I->getOpcode() == Instruction::Mul)
        return to_string(stoi(str1) * stoi(str2)); // Perform multiplication

      // Unsigned Division Instruction
      else if (I->getOpcode() == Instruction::UDiv)
        return to_string(stoi(str1) / stoi(str2)); // Perform unsigned division

      // Signed Division Instruction
      else if (I->getOpcode() == Instruction::SDiv)
        return to_string(stoi(str1) / stoi(str2)); // Perform signed division

      // Signed Remainder Instruction
      else if (I->getOpcode() == Instruction::SRem)
        return to_string(stoi(str1) % stoi(str2)); // Perform signed remainder
    }
  // If one of the inputs is "TOP", return the non "TOP" value
    else
    {
      if (str1 != "TOP")
        return str1;
      else
        return str2;
    }
  }

  bool MapCmp(map<int, string> Kill, map<int, string> Gen)
  {
    // Iterate through each key-value pair in the 'Kill' map
    for (auto &pair : Kill)
    {
      // Check if the key exists in 'Gen'
      auto itr = Gen.find(pair.first);
      if (itr != Gen.end())
      {
        // Compare the corresponding values for the current key
        if (pair.second != itr->second)
        {
          // Values are different, maps are not equal
          return false;
        }
      }
    }
    // If all keys and values are equal, maps are equal
    return true;
  }

  pair<bool, map<Instruction*, string>> RunOnBB(BasicBlock *BB, map<Instruction*, string> InSet)
  {
    // Iterate through each instruction in the basic block
    for (Instruction &I : *BB)
    {
      const Function *F = I.getFunction();

      // Handle Load Instruction
      if (LoadInst *LI = dyn_cast<LoadInst>(&I))
      {
        if (AllocaInst *AI = dyn_cast<AllocaInst>(LI->getOperand(0)))
          InSet[&I] = InSet[AI];
        
        else
        {
          if (isa<Argument>(LI->getOperand(0)))
            InSet[&I] = FunArgMap[F->getName().str()][dyn_cast<Argument>(LI->getOperand(0))->getArgNo()];
        }
      }

      // Handle Store Instruction
      if (StoreInst *SI = dyn_cast<StoreInst>(&I))
      {
        string str;

        if (isa<Constant>(SI->getOperand(0)))
        {
          Constant *c = dyn_cast<Constant>(SI->getOperand(0));
          str = to_string(c->getUniqueInteger().getSExtValue());
        }

        else if (isa<Argument>(SI->getOperand(0)))
          str = FunArgMap[F->getName().str()][dyn_cast<Argument>(SI->getOperand(0))->getArgNo()];

        else
          if (Instruction* Instr = dyn_cast<Instruction>(SI->getOperand(0)))
            str = InSet[Instr];

        // else
        // {
        //   if (isa<Argument>(SI->getOperand(0)))
        //     str = FunArgMap[F->getName().str()][dyn_cast<Argument>(SI->getOperand(0))->getArgNo()];
        // }

        Value *op0 = SI->getOperand(0);
        Value *op1 = SI->getOperand(1);

        if (isa<Argument>(SI->getOperand(1)))
          FunArgMap[F->getName().str()][dyn_cast<Argument>(SI->getOperand(1))->getArgNo()] = str;

        else
        {
          if (isa<Instruction>(SI->getOperand(1)))
            InSet[&I] = InSet[dyn_cast<Instruction>(SI->getOperand(1))] = str;
        }
      }

      // Handle Call Instruction
      if (CallInst *CI = dyn_cast<CallInst>(&I))
      {
        auto *callee = CI->getCalledFunction();
        
        // Handle printf instruction
        if (callee->getName().str() == "printf")
          InSet[CI] = "BOTTOM";

        // Handle scanf instruction
        else if (callee->getName().str() == "__isoc99_scanf")
        {
          unsigned int num = CI->getNumOperands();

          for (unsigned int i = 1; i < (num - 1); i++)
          {
            // errs() << "Operands of scanf: "<< *dyn_cast<Instruction> (CI->getOperand(i))
            if (isa<Instruction>(CI->getOperand(i)))
            {
              AllocaInst *AI = dyn_cast<AllocaInst>(CI->getOperand(i));
              InSet[AI] = "BOTTOM";
            }
            else
            {
              if (isa<Argument>(CI->getOperand(i)))
                FunArgMap[F->getName().str()][dyn_cast<Argument>(CI->getOperand(i))->getArgNo()] = "BOTTOM";
            }
          }
          InSet[CI] = "BOTTOM";
        }

        else
        {
          // Handle other call instructions
          InSet[CI] = "BOTTOM";

          map<int, string> Temp = FunArgMap[callee->getName().str()];

          unsigned int num = CI->getNumOperands();

          for (unsigned int i = 0; i < (num - 1); i++)
          {
            string str;

            if (isa<Instruction>(CI->getArgOperand(i)))
              str = InSet[dyn_cast<Instruction>(CI->getArgOperand(i))];
              
            else
            {
              if (isa<Constant>(CI->getArgOperand(i)))
              {
                Constant *c = dyn_cast<Constant>(CI->getArgOperand(i));
                str = to_string(c->getUniqueInteger().getSExtValue());
              }
            }
            FunArgMap[callee->getName().str()][i] = Intersect(FunArgMap[callee->getName().str()][i], str);
          }

          if (!MapCmp(Temp, FunArgMap[callee->getName().str()]))
          {
            int a = 0;

            CallBase *b = dyn_cast<CallBase>(CI);
            
            if (Function *Func = b->getCalledFunction())
            {
                if (a < 1)
                  FunSet.insert(Func);
                a++;
            }
          }
        }
      }

      // Handle Binary Operator
      if (BinaryOperator *Bin = dyn_cast<BinaryOperator>(&I))
      {
        string op1, op2;

        if (Constant *c = dyn_cast<Constant>(Bin->getOperand(0)))
          op1 = to_string(c->getUniqueInteger().getSExtValue());
    
        else if (Instruction *Instr = dyn_cast<Instruction>(Bin->getOperand(0)))
          op1 = InSet[Instr];

        else
        {
          if (isa<Argument>(Bin->getOperand(0)))
            op1 = FunArgMap[F->getName().str()][dyn_cast<Argument>(Bin->getOperand(0))->getArgNo()];
        }

        if (Constant *c = dyn_cast<Constant>(Bin->getOperand(1)))
          op2 = to_string(c->getUniqueInteger().getSExtValue());
        
        else if (Instruction *Instr = dyn_cast<Instruction>(Bin->getOperand(1)))
          op2 = InSet[Instr];
          
        else
        {
          if (isa<Argument>(Bin->getOperand(1)))
            op2 = FunArgMap[F->getName().str()][dyn_cast<Argument>(Bin->getOperand(1))->getArgNo()];
        }
        InSet[Bin] = Compute(&I, op1, op2);
      }
    }
    // Update the previous state and check for changes
    map<Instruction*, string> temp = Kill[BB];

    Kill[BB] = DoMeet(Kill[BB], InSet, BB);

    if (MapCmp(temp, Kill[BB]))
      return make_pair(0, InSet);
    else
      return make_pair(1, InSet);
  }

  bool runOnModule(Module &M) override
  {
    // write your code here

    // Check if the "main" function exists and add it to the FunSet
    for (Function &F : M)
    {
      // errs() << "Function : " << F.getName().str() << "\n";

      if(F.getName().str()=="main")
        FunSet.insert(&F);
    }

    // Declare a queue to hold pointers to LLVM functions
    queue<llvm::Function*> FunQ;

    for(Function &F : M)
    {
      FunQ.push(&F);

      // errs() << "Function Name: " << F.getName().str() << "\n";

      int fun = 0;

      for (Argument &Arg : F.args())
      {
        // errs() << "\nArgument Name: " << Arg.getName() << "\n";

        // Initialize argument values in the jump map to "TOP"
        FunArgMap[F.getName().str()][fun] = "TOP";

        fun++;
      }
      // errs() << "\n";        
    }

    // Process functions using Kildall's algorithm until the FunSet is empty

    // Kildall's Algoritm
    while(!FunSet.empty())
    {
      Function *F = *(FunSet.begin());

      FunSet.erase(F);

      // Initialize a worklist queue to track blocks and their analysis results
      queue<pair<BasicBlock*, map<Instruction*, string>>> Worklist;

      // Initialize a map to hold analysis results for instructions
      map<Instruction*, string> ResMap = Start(M);

      BasicBlock *BB = &F->getEntryBlock();

      // Push the entry block and its analysis results into the worklist
      Worklist.push(make_pair(BB, ResMap));

      while (!Worklist.empty())
      {
        // Get the current block and its analysis results from the front of the worklist
        pair<BasicBlock*, map<Instruction*, string>> CurrBB = Worklist.front();

        Worklist.pop();
        
        // Process the current block and update its analysis results
        pair<bool, map<Instruction*, string>> New = RunOnBB(CurrBB.first, CurrBB.second);

        // If there was a change in the analysis results
        if (New.first)
        {
          // Iterate over successors of the current block
          for (BasicBlock *S : successors(CurrBB.first))
          {
            // Add the successor block and the updated analysis results to the worklist
            Worklist.push(make_pair(S, New.second));
          }
        }
      }
    }

    // Perform Transformation in the IR file

    // Transformation
    
    // Flag to track whether a load instruction is followed by a call to scanf
    int flag = 0;
    
    // Set to keep track of instructions to delete
    set<Instruction*> DelInstr;

    for (Function &F : M)
    {
      for (BasicBlock &BB : F)
      {
        for (Instruction &I : BB)
        {
          // Check if the instruction is a binary operator or a load instruction
          if (isa<BinaryOperator>(&I) || isa<LoadInst>(&I))
          {
            flag = -1;
            
            // If the instruction is a load instruction, check if it's followed by a scanf call
            if (isa<LoadInst>(&I))
            {
              auto *LoadInstr = dyn_cast<LoadInst>(&I);

              for (auto *usr : LoadInstr->users())
              {
                if (auto *CallInstr = dyn_cast<CallInst>(usr))
                {
                  // Check if the called function is __isoc99_scanf
                  if (CallInstr->getCalledFunction()->getName().str() == "__isoc99_scanf")
                    flag = 1;
                }
              }
            }

            // Check if the previous analysis result is neither "BOTTOM" nor "TOP"
            if (Kill[&BB][&I] != "BOTTOM" && Kill[&BB][&I] != "TOP")
            {
              // If flag is -1, replace the instruction with a constant integer
              if (flag == -1)
              {
                LLVMContext &context = M.getContext();

                Value *Op1 = ConstantInt::get(Type::getInt32Ty(context), stoi(Kill[&BB][&I]));

                // Add the instruction to the set for deletion
                DelInstr.insert(&I);
                
                I.replaceAllUsesWith(Op1); // Replace uses of the instruction with the constant
              }
            }
          }
        }
      }
    }

    // Delete instructions in the DelInstr set
    for (const auto &I : DelInstr)
      I->eraseFromParent();

    return false;
  }

}; // end of struct alias_c
}  // end of anonymous namespace

// Register the pass
char constprop::ID = 0;
static RegisterPass<constprop> X("constprop", "Inter-procedural Constant Propagation Pass");

// This pass is registered with the name "constprop"
// Use this flag "constprop" to load and run this inter-procedural constant propagation pass in LLVM