function(vgt_read_file out_var path)
  file(READ "${path}" _content)
  set(${out_var} "${_content}" PARENT_SCOPE)
endfunction()
