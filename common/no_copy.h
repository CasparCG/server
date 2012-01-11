#pragma once

#define CASPAR_NO_COPY(TypeName) \
  TypeName(const TypeName&);   \
  void operator=(const TypeName&)
