#ifndef CLOUD_HANDLER_H
#define CLOUD_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

// Initializes the active cloud engine based on config.h
void cloud_handler_init(void);

// Formats and uploads data to the active cloud engine
void cloud_handler_update(void);

#ifdef __cplusplus
}
#endif

#endif // CLOUD_HANDLER_H