#ifndef LAVA_DUMP_CFG_H
#define LAVA_DUMP_CFG_H

#include "mid/ir/module.h"

#ifdef ENABLE_CFG

#include <gvc.h>
#include <iostream>
#include <sstream>
#include "common/casting.h"
#include "common/classid.h"
#include "opt/pass_manager.h"
#include "opt/analysis/dominance.h"

#endif

namespace lava::mid {

#ifdef ENABLE_CFG

enum Edge_Type {
  TRUE_TARGET, FALSE_TARGET, NO_COND
};

std::string GenerateInstructions(const std::vector<std::string> &insts) {
  std::string res = "<tr><td colspan='2'>";

  auto has = [](const std::string &str, const std::string &sub) -> bool {
    return str.find(sub) != std::string::npos;
  };

  for (const auto &inst : insts) {
    std::string cur_inst = "<font ";
    if (has(inst, "store")) {
      cur_inst += "color='deepskyblue'>";
    } else if (has(inst, "load")) {
      cur_inst += "color='green3'>";
    } else if (has(inst, "alloca")) {
      cur_inst += "color='coral'>";
    } else if (has(inst, "= phi")) {
      cur_inst += "color='purple'>";
    } else if (has(inst, "br") && has(inst, "label")) {
      cur_inst += "color='lightcoral'>";
    } else {
      cur_inst += ">";
    }

    cur_inst += inst + "</font><br/>";
    res += cur_inst;
  }

  return res + "</td></tr>";
}

Agnode_t *
MakeGraphNode(graph_t *g, const BlockPtr &block,
              IdManager &id_mgr,
              const std::vector<std::string> &insts,
              bool has_branch = false) {
  Agnode_t *n = agnode(g, nullptr, 1);

  std::string table = "<table border='0' cellborder='1' cellspacing='0'>", head = "<tr>";

  // add function in entry block
  if (block->name() == "entry") {
    head = "<tr><td colspan='2'>" + block->parent()->GetFunctionName() + "</td></tr><tr colspan='2'>";
  }

  std::stringstream ss;
  DumpBlockName(ss, id_mgr, block.get());

  //generate block head
  if (!block->empty()) {
    head += "<td>" + ss.str() + "</td><td>";
    for (const auto &pred : (*block)) {
      std::stringstream pred_ss;
      DumpBlockName(pred_ss, id_mgr, dyn_cast<BasicBlock>(pred.value()).get());
      head += pred_ss.str() + " ";
    }
    head += "</td> ";
  } else {
    head += "<td colspan='2'>" + block->name() + "</td>";
  }
  head += "</tr>";

  // generate dominance frontier
  auto dom = lava::opt::PassManager::GetAnalysis<lava::opt::DominanceInfo>("DominanceInfo")->GetDomInfo();
  auto DF = dom[block->parent().get()].DF[block.get()];
  std::string df;
  if (!DF.empty()) {
    df = "<tr><td>Dominance Frontier</td><td>";
    for (const auto &it : DF) {
      std::stringstream tmp;
      DumpBlockName(tmp, id_mgr, it);
      df += tmp.str() + " ";
    }
    df += "</td></tr>";
  }

  // generate instructions
  std::string instructions = GenerateInstructions(insts);

  // generate block tail
  std::string tail;
  if (has_branch) {
    tail += "<tr>"
            "<td port='f0'>true</td>"
            "<td port='f1'>false</td>"
            "</tr>";
  }

  table += head + df + instructions + tail + "</table>";


//  printf("%s\n\n", table.c_str());
  char *content = agstrdup_html(agroot(n), table.c_str());
  agsafeset((void *) n, (char *) "label", content, "");
  agsafeset((void *) n, (char *) "shape", "none", "");
  agsafeset((void *) n, (char *) "fontname", "Cambria Math", "");
  agstrfree(g, content);
  //agsafeset((void *)n, "fontname", "Comic Sans", "");

  return n;
}

void AddEdge(graph_t *g, Agnode_t *from, Agnode_t *to, Edge_Type type) {
  Agedge_t *e = agedge(g, from, to, nullptr, 1);

  if (type != NO_COND) {
    agsafeset((void *) e, (char *) "tailport", (type == TRUE_TARGET) ? "f0" : "f1", "");
    agsafeset((void *) e, (char *) "color", (type == TRUE_TARGET) ? "green" : "red", "");
  } else {
    agsafeset((void *) e, (char *) "tailport", "s", "");

  }
}

#endif

void MakeGlobalVariables(graph_t *g, Module *module, IdManager &id_mgr) {
#ifdef ENABLE_CFG
  Agnode_t *n = agnode(g, nullptr, 1);
  std::string table = "<table border='0' cellborder='1' cellspacing='0'>";

  std::string head = "<tr>"
                     "<td colspan='3'>Global Value</td>"
                     "</tr>";
  head += "<tr>"
          "<td><b>name</b></td>"
          "<td><b>type</b></td>"
          "<td><b>init</b></td>"
          "</tr>";

  std::string values;
  for (const auto &it : module->GlobalVars()) {
    std::string value;

    // variable name
    auto var = dyn_cast<GlobalVariable>(it);
    value = "<tr><td>" + var->name() + "</td>";

    // variable type
    value += "<td>" + var->type()->GetTypeId() + "</td>";

    // init
    if (var->init()) {
      std::stringstream tmp;
      if (auto const_array = dyn_cast<ConstantArray>(var->init())) {
        const_array->Dump(tmp, id_mgr, "");
      } else {
        var->init()->Dump(tmp, id_mgr);
      }
      value += "<td>" + tmp.str() + "</td>";
    } else {
      value += "<td>zeroinitializer</td>";
    }
    values += value + "</tr>";
  }

  table += head + values + "</table>";
  char *content = agstrdup_html(agroot(n), table.c_str());
  agsafeset((void *) n, (char *) "label", content, "");
  agsafeset((void *) n, (char *) "shape", "none", "");
  agsafeset((void *) n, (char *) "fontname", "Cambria Math", "");
  agstrfree(g, content);
#endif

}


void MakeCFG(graph_t *g, const FuncPtr &func, IdManager &id_mgr) {

#ifdef ENABLE_CFG
  // make CFG nodes
  std::unordered_map<SSAPtr, Agnode_t *> nodes;
  for (const auto &it : (*func)) {
    std::string content;
    std::stringstream ss;
    auto block = dyn_cast<BasicBlock>(it.value());

    block->Dump(ss, id_mgr, "\\");
    content = ss.str();

    std::vector<std::string> insts;
    std::string inst;
    for (const auto &ch : content) {
      if (ch == '\\') {
        insts.push_back(inst);
        inst = "";
      } else inst += ch;
    }

    bool has_branch = IsSSA<BranchInst>(block->insts().back());
    nodes.emplace(block, MakeGraphNode(g, block, id_mgr, insts, has_branch));
  }

  // connect the nodes
  for (const auto &it : (*func)) {
    auto block = dyn_cast<BasicBlock>(it.value());
    for (const auto &pred : (*block)) {
      auto parent_bb = dyn_cast<BasicBlock>(pred.value());
      auto parent_node = nodes[parent_bb];
      auto child_node = nodes[block];
      Edge_Type type = NO_COND;

      if (!parent_bb->insts().empty()) {
        auto last_inst = parent_bb->insts().back();
        bool has_branch = IsSSA<BranchInst>(last_inst);
        if (has_branch) {
          type = (dyn_cast<BranchInst>(last_inst)->true_block() == block) ? TRUE_TARGET : FALSE_TARGET;
        }
      }

      AddEdge(g, parent_node, child_node, type);
    }
  }
#endif

}

}

//}

#endif //LAVA_DUMP_CFG_H

