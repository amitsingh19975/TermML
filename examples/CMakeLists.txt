function (add_exec source output)
  add_executable(${output} ${source})
  target_link_libraries(${output} PRIVATE termml_project_options termml_project_warnings termml_core)
endfunction()

add_exec("main.cpp" main)
