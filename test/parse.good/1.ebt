# TODOXXX needs a lot more examples
probe insn {}
probe insn ($opcode == "div") {}
probe function ($name == "foo") and insn ($opcode == "div") {}
probe insn and not function ($name == "boring") {}
