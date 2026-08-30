/**
 * @file api.h
 * @author 陈林锴
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct spiration_api_handle spiration_api_handle;

spiration_api_handle* api_create(void);
void api_destroy(spiration_api_handle* handle);

void api_do_something(spiration_api_handle* handle, int param);
int api_get_value(spiration_api_handle* handle);

#ifdef __cplusplus
}
#endif
