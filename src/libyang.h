/**
 * @file libyang.h
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief The main libyang public header.
 *
 * Copyright (c) 2015 - 2018 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef LY_LIBYANG_H_
#define LY_LIBYANG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup context Context
 * @{
 *
 * Structures and functions to manipulate with the libyang "containers". The \em context concept allows callers
 * to work in environments with different sets of YANG schemas. More detailed information can be found at
 * @ref howtocontext page.
 */

/**
 * @struct ly_ctx
 * @brief libyang context handler.
 */
struct ly_ctx;

/**@} context */

#include "log.h"
#include "set.h"
#include "dict.h"
#include "tree_schema.h"

/**
 * @ingroup context
 * @{
 */

/**
 * @defgroup contextoptions Context options
 * @ingroup context
 *
 * Options to change context behavior.
 * @{
 */

#define LY_CTX_ALLIMPLEMENTED 0x01 /**< All the imports of the schema being parsed are treated implemented. */
#define LY_CTX_TRUSTED        0x02 /**< Handle the schema being parsed as trusted and skip its validation
                                        tests. Note that while this option improves performance, it can
                                        lead to an undefined behavior if the schema is not correct. */
#define LY_CTX_NOYANGLIBRARY  0x04 /**< Do not internally implement ietf-yang-library module. The option
                                        causes that function ly_ctx_info() does not work (returns NULL) until
                                        the ietf-yang-library module is loaded manually. While any revision
                                        of this schema can be loaded with this option, note that the only
                                        revisions implemented by ly_ctx_info() are 2016-04-09 and 2018-01-17.
                                        This option cannot be changed on existing context. */
#define LY_CTX_DISABLE_SEARCHDIRS 0x08  /**< Do not search for schemas in context's searchdirs neither in current
                                        working directory. It is entirely skipped and the only way to get
                                        schema data for imports or for ly_ctx_load_module() is to use the
                                        callbacks provided by caller via ly_ctx_set_module_imp_clb() */
#define LY_CTX_DISABLE_SEARCHDIR_CWD 0x10 /**< Do not automatically search for schemas in current working
                                        directory, which is by default searched automatically (despite not
                                        recursively). */
#define LY_CTX_PREFER_SEARCHDIRS 0x20 /**< When searching for schema, prefer searchdirs instead of user callback. */
/**@} contextoptions */

/**
 * @brief Create libyang context.
 *
 * Context is used to hold all information about schemas. Usually, the application is supposed
 * to work with a single context in which libyang is holding all schemas (and other internal
 * information) according to which the data trees will be processed and validated. So, the schema
 * trees are tightly connected with the specific context and they are held by the context internally
 * - caller does not need to keep pointers to the schemas returned by lys_parse(), context knows
 * about them. The data trees created with lyd_parse() are still connected with the specific context,
 * but they are not internally held by the context. The data tree just points and lean on some data
 * held by the context (schema tree, string dictionary, etc.). Therefore, in case of data trees, caller
 * is supposed to keep pointers returned by the lyd_parse() and manage the data tree on its own. This
 * also affects the number of instances of both tree types. While you can have only one instance of
 * specific schema connected with a single context, number of data tree instances is not connected.
 *
 * @param[in] search_dir Directory where libyang will search for the imported or included modules
 * and submodules. If no such directory is available, NULL is accepted.
 * @param[in] options Context options, see @ref contextoptions.
 * @param[out] new_ctx Pointer to the created libyang context if LY_SUCCESS returned.
 * @return LY_ERR return value.
 */
LY_ERR ly_ctx_new(const char *search_dir, int options, struct ly_ctx **new_ctx);

/**
 * @brief Add the search path into libyang context
 *
 * To reset search paths set in the context, use ly_ctx_unset_searchdirs() and then
 * set search paths again.
 *
 * @param[in] ctx Context to be modified.
 * @param[in] search_dir New search path to add to the current paths previously set in ctx.
 * @return LY_ERR return value.
 */
LY_ERR ly_ctx_set_searchdir(struct ly_ctx *ctx, const char *search_dir);

/**
 * @brief Clean the search path(s) from the libyang context
 *
 * @param[in] ctx Context to be modified.
 * @param[in] value Searchdir to be removed, use NULL to remove them all.
 * @return LY_ERR return value
 */
LY_ERR ly_ctx_unset_searchdirs(struct ly_ctx *ctx, const char *value);

/**
 * @brief Get the NULL-terminated list of the search paths in libyang context. Do not modify the result!
 *
 * @param[in] ctx Context to query.
 * @return NULL-terminated list (array) of the search paths, NULL if no searchpath was set.
 * Do not modify the provided data in any way!
 */
const char * const *ly_ctx_get_searchdirs(const struct ly_ctx *ctx);

/**
 * @brief Get the currently set context's options.
 *
 * @param[in] ctx Context to query.
 * @return Combination of all the currently set context's options, see @ref contextoptions.
 */
int ly_ctx_get_options(const struct ly_ctx *ctx);

/**
 * @brief Set some of the context's options, see @ref contextoptions.
 * @param[in] ctx Context to be modified.
 * @param[in] option Combination of the context's options to be set, see @ref contextoptions.
 * @return LY_ERR value.
 */
LY_ERR ly_ctx_set_option(struct ly_ctx *ctx, int option);

/**
 * @brief Unset some of the context's options, see @ref contextoptions.
 * @param[in] ctx Context to be modified.
 * @param[in] option Combination of the context's options to be unset, see @ref contextoptions.
 * @return LY_ERR value.
 */
LY_ERR ly_ctx_unset_option(struct ly_ctx *ctx, int option);

/**
 * @brief Get current ID of the modules set. The value is available also
 * as module-set-id in ly_ctx_info() result.
 *
 * @param[in] ctx Context to be examined.
 * @return Numeric identifier of the current context's modules set.
 */
uint16_t ly_ctx_get_module_set_id(const struct ly_ctx *ctx);

/**
 * @brief Free all internal structures of the specified context.
 *
 * The function should be used before terminating the application to destroy
 * and free all structures internally used by libyang. If the caller uses
 * multiple contexts, the function should be called for each used context.
 *
 * All instance data are supposed to be freed before destroying the context.
 * Data models are destroyed automatically as part of ly_ctx_destroy() call.
 *
 * @param[in] ctx libyang context to destroy
 * @param[in] private_destructor Optional destructor function for private objects assigned
 * to the nodes via lys_set_private(). If NULL, the private objects are not freed by libyang.
 * Remember the differences between the structures derived from ::lys_node and always check
 * ::lys_node#nodetype.
 */
void ly_ctx_destroy(struct ly_ctx *ctx, void (*private_destructor)(const struct lysc_node *node, void *priv));

/** @} context */

#ifdef __cplusplus
}
#endif

#endif /* LY_LIBYANG_H_ */