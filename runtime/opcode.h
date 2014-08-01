/* XXX requires dr_api.h to have been included previously */

/* TODOXXX placeholder for a much larger opcode table? */
const char *s_OP_div = "div";
const char *s_OP_unknown = "<unknown>";

const char *
opcode_string(int opcode)
{
  switch(opcode)
    {
    case OP_div: return s_OP_div;
    default: return s_OP_unknown;
    }
}
