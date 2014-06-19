probe function ($name == "foo") :: entry { }
probe function ($name != "foo") :: insn ($opcode == "div") { }
