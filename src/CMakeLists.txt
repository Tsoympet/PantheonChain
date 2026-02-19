add_library(pantheon_layers STATIC
    common/commitments.cpp
    talanton/l1_commitment_validator.cpp
    drachma/pos_consensus.cpp
    obolos/execution.cpp
)

target_include_directories(pantheon_layers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
  target_compile_options(pantheon_layers PRIVATE /W4 /WX)
else()
  target_compile_options(pantheon_layers PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
