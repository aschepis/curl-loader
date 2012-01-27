#include <vscript.h>
#include "rtlapi.h"


int std_length(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	int len = -1;
	VM_BASE_TYPE  var_type = VM_TYPE_get_base_type( params->param[0] );
	
	V_UNUSED(method_id);
	
	if (VM_IS_SCALAR( var_type ) ) {
		char sSize[100];

		VM_SCALAR_to_string(ctx, params->param[0], sSize, &len);
	} else if (var_type == VM_ARRAY) {
		VM_VALUE_ARRAY * arr = ((VM_VALUE_ARRAY *) params->param[0]);
		len = arr -> count; 
	} else if (var_type == VM_HASH) {
		VM_VALUE_HASH * hash = ((VM_VALUE_HASH *) params->param[0]);
		len = hash -> imp.elmcount;
	}

	params->retval = VM_VALUE_LONG_return(ctx, len);
	
	return 0;
}

