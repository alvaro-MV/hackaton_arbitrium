#include "../include/stylus_sdk.h"
#include "../stylus-sdk-c/include/stylus_utils.h"
#include "../stylus-sdk-c/include/storage.h"
#include "../stylus-sdk-c/include/string.h"

#define STORAGE_SLOT__base 0x0 // Dirección base para almacenar datos
#define STORAGE_SLOT__index 0x1000 // Dirección para almacenar el índice actual

// Buffer para manejar datos
struct {
    uint8_t datas[10240]; // Hasta 10 KB de datos por entrada
    uint8_t id_dream;
} dream_t;

// Función para devolver un resultado exitoso
ArbResult inline _return_success_bebi32(bebi32 const retval)
{
    ArbResult res = {Success, retval, 32};
    return res;
}

// Función para obtener el índice actual
uint32_t get_current_index()
{
    uint32_t index = 0;
    storage_load(STORAGE_SLOT__index, (uint8_t *)&index, sizeof(index));
    return index;
}

// Función para incrementar el índice
void increment_index()
{
    uint32_t index = get_current_index();
    index++;
    storage_store(STORAGE_SLOT__index, (uint8_t *)&index, sizeof(index));
}

// Función para agregar datos a la matriz
ArbResult add_data(uint8_t *input, size_t len)
{
    if (len > 10240) // Limitar el tamaño a 10 KB por entrada
    {
        return _return_short_string(Failure, "Exceeds10KB");
    }

    uint32_t index = get_current_index(); // Obtener el índice actual
    uint8_t *slot_address = (uint8_t *)(STORAGE_SLOT__base + (index * 10240)); // Calcular la dirección de almacenamiento

    // Guardar los datos en el almacenamiento persistente
    storage_store(slot_address, input, len);

    // Incrementar el índice para la próxima entrada
    increment_index();

    return _return_success_bebi32(index);
}

// Función para obtener datos de la matriz
ArbResult get_data(uint8_t *input, size_t len)
{
    if (len != sizeof(uint32_t)) // Verificar que la entrada sea un índice válido
    {
        return _return_short_string(Failure, "InvalidInput");
    }

    uint32_t index = *((uint32_t *)input); // Extraer el índice solicitado
    uint32_t current_index = get_current_index();

    if (index >= current_index) // Verificar que el índice esté dentro del rango
    {
        return _return_short_string(Failure, "IndexOutOfRange");
    }

    uint8_t *slot_address = (uint8_t *)(STORAGE_SLOT__base + (index * 10240)); // Calcular la dirección de almacenamiento
    storage_load(slot_address, dream_t.datas, 10240);

    // Retornar los datos almacenados
    ArbResult res = {Success, dream_t.datas, 10240};
    return res;
}

int handler(size_t argc)
{
    // Leer los argumentos de la llamada
    uint8_t argv[argc];
    read_args(argv);

    // Registrar las funciones disponibles
    FunctionRegistry registry[] = {
        {to_function_selector("add_data(bytes)"), add_data},
        {to_function_selector("get_data(uint256)"), get_data},
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
