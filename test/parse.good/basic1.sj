probe insn {}
probe insn ($opcode == "div") {}
probe insn ($fname == "foo", $opcode == "div") {}
probe insn ($fname != "boring") {}
probe fcall ($name == "foo") {}