#ifndef LAVA_CLASSID_H
#define LAVA_CLASSID_H

namespace lava {

enum class ClassId {
#define CLASS_ID(id, opcode, name) name##Id = id,
#include "classId.inc"
};

}

#endif //LAVA_CLASSID_H
