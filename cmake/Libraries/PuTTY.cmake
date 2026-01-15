#-------------------------------------------------------------------------------
# PuTTY Library Configuration
#
# This module provides functions and variables for building PuTTY libraries
# (putty and puttyvs) as part of NetBox project.
#
# NOTE: Paths are relative to libs/PuTTY/ directory where this file is included.
#-------------------------------------------------------------------------------

# Set PuTTY compile flags
set(PUTTY_COMPILE_FLAGS
    -DLibrary
    -D_MT
    -DMPEXT
    -DNET_SETUP_DIAGNOSTICS
    -DSECURITY_WIN32
    -DWINSCP
    -DMPEXT
    -DPLATFORM_HAS_SMEMCLR
    -D_WINDOWS
)

if(MSVC)
    set(PUTTY_COMPILE_FLAGS ${PUTTY_COMPILE_FLAGS}
        -wd4996
        -wd4068  # warning C4068: unknown pragma 'warn'
    )
endif()

#-------------------------------------------------------------------------------
# Function: putty_apply_compile_options
#
# Applies common PuTTY compile options to a target
#
# Arguments:
#   TARGET - The CMake target to apply options to
#-------------------------------------------------------------------------------
function(putty_apply_compile_options TARGET)
    target_compile_options(${TARGET} PRIVATE
        ${PUTTY_COMPILE_FLAGS}
        -Dssize_t=size_t
        ${NETBOX_C_FLAGS}
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: puttyvs_apply_compile_options
#
# Applies compile options to puttyvs target (vectorized AES/SHA)
#
# Arguments:
#   TARGET - The CMake target to apply options to
#-------------------------------------------------------------------------------
function(puttyvs_apply_compile_options TARGET)
    target_compile_options(${TARGET} PRIVATE
        ${PUTTY_COMPILE_FLAGS}
        -Dssize_t=size_t
        ${NETBOX_C_FLAGS}
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: putty_get_sources
#
# Returns the list of PuTTY source files for the main putty library
#
# Output:
#   PUTTY_SOURCES - List of source files (relative to libs/PuTTY/)
#-------------------------------------------------------------------------------
function(putty_get_sources PUTTY_SOURCES)
    set(${PUTTY_SOURCES}
        be_list.c
        callback.c
        crypto/aes-common.c
        crypto/aesold.c
        crypto/aes-select.c
        crypto/aes-sw.c
        crypto/arcfour.c
        crypto/argon2.c
        crypto/bcrypt.c
        crypto/blake2.c
        crypto/blowfish.c
        crypto/chacha20-poly1305.c
        crypto/des.c
        crypto/diffie-hellman.c
        crypto/dsa.c
        crypto/ecc-arithmetic.c
        crypto/ecc-ssh.c
        crypto/hash_simple.c
        crypto/hmac.c
        crypto/mac.c
        crypto/mac_simple.c
        crypto/md5.c
        crypto/mpint.c
        crypto/prng.c
        crypto/pubkey-pem.c
        crypto/pubkey-ppk.c
        crypto/rfc6979.c
        crypto/rsa.c
        crypto/sha1-common.c
        crypto/sha1-select.c
        crypto/sha1-sw.c
        crypto/sha256-common.c
        crypto/sha256-select.c
        crypto/sha256-sw.c
        crypto/sha3.c
        crypto/sha512-common.c
        crypto/sha512-select.c
        crypto/sha512-sw.c
        crypto/kex-hybrid.c
        crypto/mlkem.c
        crypto/aesgcm-footer.h
        crypto/aesgcm-ref-poly.c
        crypto/aesgcm-select.c
        crypto/aesgcm-sw.c
        crypto/aesgcm.h
        crypto/ntru.c
        crypto/openssh-certs.c
        errsock.c
        import.c
        logging.c
        proxy/cproxy.c
        proxy/http_p.c
        proxy/interactor.c
        proxy/local.c
        proxy/nosshproxy.c
        proxy/proxy.c
        proxy/socks4.c
        proxy/socks5.c
        proxy/telnet.c
        settings.c
        ssh/agentf.c
        ssh/bpp2.c
        ssh/bpp-bare.c
        ssh/censor2.c
        ssh/common_p.c
        ssh/connection2.c
        ssh/connection2-client.c
        ssh/gssc.c
        ssh/kex2-client.c
        ssh/mainchan.c
        ssh/nosharing.c
        ssh/pgssapi.c
        ssh/portfwd.c
        ssh/sharing.c
        ssh/ssh.c
        ssh/transient-hostkey-cache.c
        ssh/transport2.c
        ssh/userauth2-client.c
        ssh/verstring.c
        ssh/zlib.c
        sshpubk.c
        sshrand.c
        utils/antispoof.c
        utils/backend_socket_log.c
        utils/base64_decode_atom.c
        utils/base64_encode_atom.c
        utils/bufchain.c
        utils/burnstr.c
        utils/conf.c
        utils/conf_launchable.c
        utils/ctrlparse.c
        utils/default_description.c
        utils/dup_mb_to_wc.c
        utils/dupcat.c
        utils/dupprintf.c
        utils/dupstr.c
        utils/host_strchr.c
        utils/host_strchr_internal.c
        utils/host_strcspn.c
        utils/host_strduptrim.c
        utils/host_strrchr.c
        utils/log_proxy_stderr.c
        utils/make_spr_sw_abort_static.c
        utils/marshal.c
        utils/memory.c
        utils/memxor.c
        utils/nullstrcmp.c
        utils/out_of_memory.c
        utils/parse_blocksize.c
        utils/prompts.c
        utils/ptrlen.c
        utils/seat_connection_fatal.c
        utils/sk_free_peer_info.c
        utils/smemclr.c
        utils/smemeq.c
        utils/spr_get_error_message.c
        utils/ssh2_pick_fingerprint.c
        utils/sshutils.c
        utils/strbuf.c
        utils/string_length_for_printf.c
        utils/stripctrl.c
        utils/tempseat.c
        utils/tree234.c
        utils/version.c
        utils/wildcard.c
        utils/write_c_string_literal.c
        utils/conf_dest.c
        utils/base64_valid.c
        utils/decode_utf8.c
        utils/decode_utf8_to_wchar.c
        utils/decode_utf8_to_wide_string.c
        utils/dup_wc_to_mb.c
        utils/dupwcs.c
        utils/encode_utf8.c
        utils/encode_wide_string_as_utf8.c
        utils/logeventf.c
        utils/conf_data.c
        utils/burnwcs.c
        utils/base64_decode.c
        utils/base64_encode.c
        utils/cert-expr.c
        utils/host_ca_new_free.c
        utils/key_components.c
        utils/seat_dialog_text.c
        stubs/null-lp.c
        stubs/null-mac.c
        stubs/null-opener.c
        stubs/null-plug.c
        stubs/null-seat.c
        stubs/null-cipher.c
        stubs/null-key.c
        stubs/null-socket.c
        windows/agent-client.c
        windows/gss.c
        windows/handle-io.c
        windows/handle-socket.c
        windows/handle-wait.c
        windows/local-proxy.c
        windows/named-pipe-client.c
        windows/network.c
        windows/noise.c
        windows/no-jump-list.c
        windows/storage.c
        windows/unicode.c
        windows/utils/agent_named_pipe_name.c
        windows/utils/cryptoapi.c
        windows/utils/defaults.c
        windows/utils/escape_registry_key.c
        windows/utils/filename.c
        windows/utils/fontspec.c
        windows/utils/get_system_dir.c
        windows/utils/get_username.c
        windows/utils/load_system32_dll.c
        windows/utils/ltime.c
        windows/utils/open_for_write_would_lose_data.c
        windows/utils/security_p.c
        windows/utils/win_strerror.c
        windows/utils/registry.c
        PARENT_SCOPE
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: puttyvs_get_sources
#
# Returns the list of PuTTY source files for the vectorized puttyvs library
#
# Output:
#   PUTTYVS_SOURCES - List of source files (relative to libs/PuTTY/)
#-------------------------------------------------------------------------------
function(puttyvs_get_sources PUTTYVS_SOURCES)
    set(LOCAL_SOURCES
        crypto/aes-sw.c
        crypto/argon2.c
        crypto/sha256-sw.c
    )

    if(NOT PROJECT_PLATFORM STREQUAL "ARM64")
        list(APPEND LOCAL_SOURCES crypto/aes-ni.c)
    endif()

    # Update the parent scope variable
    set(${PUTTYVS_SOURCES} ${LOCAL_SOURCES} PARENT_SCOPE)
endfunction()
