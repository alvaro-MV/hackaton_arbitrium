#include "../include/stylus_sdk.h"
#include "../stylus-sdk-c/include/stylus_utils.h"
#include "../stylus-sdk-c/include/storage.h"
#include "../stylus-sdk-c/include/string.h"

#define STORAGE_SLOT__value 0x0
#define STORAGE_SLOT__index 0x1000 // Dirección para almacenar el índice actual

// buffer used to write output, avoiding malloc
struct {
  uint8_t datas[32];
  uint8_t id_dream;
} dream_t;


uint32_t get_current_index()
{
    uint32_t index = 0;
    storage_load(STORAGE_SLOT__index, (uint8_t *)&index, sizeof(index));
    return index;
}
// succeed and return a bebi32
ArbResult inline _return_success_bebi32(bebi32 const retval)
{
  ArbResult res = {Success, retval, 32};
  return res;
}

ArbResult set_value(uint8_t *input, size_t len)
{
  if (len != 32)
  {
    // revert if input length is not 32 bytes
    return _return_short_string(Failure, "InvalidLength");
  }

  uint8_t *slot_address = (uint8_t *)(STORAGE_SLOT__value + 0); // Get the slot address

  // Allocate a temporary buffer to store the input
  storage_cache_bytes32(slot_address, input);

  // Flush the cache to store the value permanently
  storage_flush_cache(false);
  return _return_success_bebi32(input);
}

// 
ArbResult get_value(uint8_t *input, size_t len)
{
  uint32_t index = *((uint32_t *)input); // Extraer el índice solicitado
  uint32_t current_index = get_current_index();

  if (index >= current_index) // Verificar que el índice esté dentro del rango
  {
      return _return_short_string(Failure, "IndexOutOfRange");
  }

  uint8_t *slot_address = (uint8_t *)(STORAGE_SLOT__value + (index * 10240)); // Calcular la dirección de almacenamiento

  storage_load_bytes32(slot_address, dream_t.datas);
  if (bebi32_is_zero(dream_t.datas))
  {
    return _return_short_string(Failure, "NotSet");
  }

  return _return_success_bebi32(dream_t.datas);
}


int handler(size_t argc)
{
  // Save the function calldata
  uint8_t argv[argc];
  read_args(argv); // 4 bytes for selector + function arguments

  // Define the registry array with registered functions
  FunctionRegistry registry[] = {
      {to_function_selector("set_value(uint256)"), set_value},
      {to_function_selector("get_value()"), get_value},
  };

  uint32_t signature = *((uint32_t *)argv); // Selector de función

  // Llamar a la función basada en el selector
  ArbResult res = call_function(
      registry,
      sizeof(registry) / sizeof(registry[0]),
      signature,
      argv + 4,
      argc - 4
  );
  return (write_result(res.output, res.output_len), res.status);
}

ENTRYPOINT(handler)