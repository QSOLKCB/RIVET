/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/document.h"

#include <limits.h>

static int doc_ascii_equal_ci(
    const unsigned char *bytes,
    size_t length,
    const unsigned char *expected,
    size_t expected_length
)
{
    size_t i;

    if (length != expected_length) {
        return 0;
    }

    for (i = 0u; i < length; ++i) {
        unsigned char left = bytes[i];
        unsigned char right = expected[i];

        if (left >= 0x41u && left <= 0x5au) {
            left = (unsigned char)(left + 0x20u);
        }
        if (right >= 0x41u && right <= 0x5au) {
            right = (unsigned char)(right + 0x20u);
        }
        if (left != right) {
            return 0;
        }
    }

    return 1;
}

static int doc_hex(unsigned char byte)
{
    return (byte >= 0x30u && byte <= 0x39u) ||
           (byte >= 0x41u && byte <= 0x46u) ||
           (byte >= 0x61u && byte <= 0x66u);
}

static int doc_url_host_byte(unsigned char byte)
{
    return (byte >= 0x30u && byte <= 0x39u) ||
           (byte >= 0x41u && byte <= 0x5au) ||
           (byte >= 0x61u && byte <= 0x7au) ||
           byte == 0x2du ||
           byte == 0x2eu;
}

static rivet_result doc_url_validate_host(
    const unsigned char *bytes,
    size_t start,
    size_t end
)
{
    size_t label_start = start;
    size_t i;

    if (start == end) {
        return RIVET_ERR_UNSUPPORTED;
    }

    for (i = start; i < end; ++i) {
        if (bytes[i] != 0x2eu) {
            continue;
        }

        if (i == label_start ||
            bytes[label_start] == 0x2du ||
            bytes[i - 1u] == 0x2du) {
            return RIVET_ERR_UNSUPPORTED;
        }

        label_start = i + 1u;
    }

    if (label_start == end) {
        return RIVET_OK;
    }

    if (bytes[label_start] == 0x2du ||
        bytes[end - 1u] == 0x2du) {
        return RIVET_ERR_UNSUPPORTED;
    }

    return RIVET_OK;
}

static rivet_result doc_utf8_one(
    const unsigned char *bytes,
    size_t byte_count,
    size_t offset,
    unsigned int *codepoint,
    size_t *used
)
{
    unsigned char first;
    unsigned int value;
    size_t need;
    size_t i;

    if (bytes == NULL ||
        offset >= byte_count ||
        codepoint == NULL ||
        used == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    first = bytes[offset];

    if (first <= 0x7fu) {
        *codepoint = (unsigned int)first;
        *used = 1u;
        return RIVET_OK;
    }

    if (first >= 0xc2u && first <= 0xdfu) {
        value = (unsigned int)(first & 0x1fu);
        need = 2u;
    } else if (first >= 0xe0u && first <= 0xefu) {
        value = (unsigned int)(first & 0x0fu);
        need = 3u;
    } else if (first >= 0xf0u && first <= 0xf4u) {
        value = (unsigned int)(first & 0x07u);
        need = 4u;
    } else {
        return RIVET_ERR_UNSUPPORTED;
    }

    if (need > byte_count - offset) {
        return RIVET_ERR_UNSUPPORTED;
    }

    for (i = 1u; i < need; ++i) {
        unsigned char continuation = bytes[offset + i];

        if ((continuation & 0xc0u) != 0x80u) {
            return RIVET_ERR_UNSUPPORTED;
        }
        value = (value << 6u) |
                (unsigned int)(continuation & 0x3fu);
    }

    if ((need == 3u &&
         ((first == 0xe0u && bytes[offset + 1u] < 0xa0u) ||
          (first == 0xedu && bytes[offset + 1u] >= 0xa0u))) ||
        (need == 4u &&
         ((first == 0xf0u && bytes[offset + 1u] < 0x90u) ||
          (first == 0xf4u && bytes[offset + 1u] >= 0x90u))) ||
        value > 0x10ffffu ||
        (value >= 0xd800u && value <= 0xdfffu)) {
        return RIVET_ERR_UNSUPPORTED;
    }

    *codepoint = value;
    *used = need;
    return RIVET_OK;
}

rivet_result rivet_byte_stream_init(
    rivet_byte_stream *stream,
    const unsigned char *bytes,
    size_t length
)
{
    if (stream == NULL ||
        (length != 0u && bytes == NULL)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    stream->bytes = bytes;
    stream->length = length;
    stream->offset = 0u;
    return RIVET_OK;
}

rivet_result rivet_byte_stream_read(
    rivet_byte_stream *stream,
    unsigned char *buffer,
    size_t capacity,
    size_t requested,
    size_t *read_count
)
{
    size_t available;
    size_t count;
    size_t i;

    if (stream == NULL ||
        stream->offset > stream->length ||
        (stream->length != 0u && stream->bytes == NULL) ||
        read_count == NULL ||
        requested > capacity ||
        (requested != 0u && buffer == NULL)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    available = stream->length - stream->offset;
    count = requested < available ? requested : available;

    for (i = 0u; i < count; ++i) {
        buffer[i] = stream->bytes[stream->offset + i];
    }

    stream->offset += count;
    *read_count = count;
    return RIVET_OK;
}

rivet_result rivet_byte_stream_peek(
    const rivet_byte_stream *stream,
    unsigned char *byte
)
{
    if (stream == NULL ||
        byte == NULL ||
        stream->offset > stream->length ||
        (stream->length != 0u && stream->bytes == NULL)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (stream->offset == stream->length) {
        return RIVET_ERR_NOT_FOUND;
    }

    *byte = stream->bytes[stream->offset];
    return RIVET_OK;
}

size_t rivet_byte_stream_remaining(
    const rivet_byte_stream *stream
)
{
    if (stream == NULL ||
        stream->offset > stream->length) {
        return 0u;
    }
    return stream->length - stream->offset;
}

rivet_result rivet_url_parse(
    rivet_url *url,
    const unsigned char *bytes,
    size_t byte_count
)
{
    static const unsigned char http_scheme[] =
        {0x68u,0x74u,0x74u,0x70u};
    static const unsigned char https_scheme[] =
        {0x68u,0x74u,0x74u,0x70u,0x73u};
    size_t i;
    size_t scheme_end = (size_t)-1;
    size_t host_start;
    size_t host_end;
    size_t cursor;
    size_t path_start;
    size_t path_end;
    size_t query_start = 0u;
    size_t query_end = 0u;
    size_t fragment_start = 0u;
    unsigned int port = 0u;
    int secure = 0;
    int has_port = 0;
    rivet_url candidate;

    if (url == NULL ||
        bytes == NULL ||
        byte_count == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (i = 0u; i < byte_count; ++i) {
        unsigned char byte = bytes[i];

        if (byte < 0x21u || byte > 0x7eu) {
            return RIVET_ERR_UNSUPPORTED;
        }
        if (byte == 0x25u) {
            if (i + 2u >= byte_count ||
                !doc_hex(bytes[i + 1u]) ||
                !doc_hex(bytes[i + 2u])) {
                return RIVET_ERR_UNSUPPORTED;
            }
            i += 2u;
        }
    }

    for (i = 0u; i < byte_count; ++i) {
        if (bytes[i] == 0x3au) {
            scheme_end = i;
            break;
        }
        if (!((bytes[i] >= 0x41u && bytes[i] <= 0x5au) ||
              (bytes[i] >= 0x61u && bytes[i] <= 0x7au))) {
            return RIVET_ERR_UNSUPPORTED;
        }
    }

    if (scheme_end == (size_t)-1 ||
        scheme_end == 0u ||
        scheme_end + 2u >= byte_count ||
        bytes[scheme_end + 1u] != 0x2fu ||
        bytes[scheme_end + 2u] != 0x2fu) {
        return RIVET_ERR_UNSUPPORTED;
    }

    if (doc_ascii_equal_ci(
            bytes,
            scheme_end,
            http_scheme,
            sizeof(http_scheme))) {
        secure = 0;
        port = 80u;
    } else if (doc_ascii_equal_ci(
                   bytes,
                   scheme_end,
                   https_scheme,
                   sizeof(https_scheme))) {
        secure = 1;
        port = 443u;
    } else {
        return RIVET_ERR_UNSUPPORTED;
    }

    host_start = scheme_end + 3u;
    host_end = host_start;
    while (host_end < byte_count &&
           bytes[host_end] != 0x3au &&
           bytes[host_end] != 0x2fu &&
           bytes[host_end] != 0x3fu &&
           bytes[host_end] != 0x23u) {
        if (!doc_url_host_byte(bytes[host_end])) {
            return RIVET_ERR_UNSUPPORTED;
        }
        ++host_end;
    }

    if (doc_url_validate_host(
            bytes,
            host_start,
            host_end) != RIVET_OK) {
        return RIVET_ERR_UNSUPPORTED;
    }

    cursor = host_end;
    if (cursor < byte_count &&
        bytes[cursor] == 0x3au) {
        unsigned int parsed_port = 0u;
        int digits = 0;

        has_port = 1;
        ++cursor;
        while (cursor < byte_count &&
               bytes[cursor] >= 0x30u &&
               bytes[cursor] <= 0x39u) {
            unsigned int digit =
                (unsigned int)(bytes[cursor] - 0x30u);

            if (parsed_port > 6553u ||
                (parsed_port == 6553u && digit > 5u)) {
                return RIVET_ERR_UNSUPPORTED;
            }
            parsed_port = parsed_port * 10u + digit;
            ++cursor;
            digits = 1;
        }

        if (!digits || parsed_port == 0u) {
            return RIVET_ERR_UNSUPPORTED;
        }
        port = parsed_port;
    }

    path_start = cursor;
    if (cursor < byte_count &&
        bytes[cursor] == 0x2fu) {
        while (cursor < byte_count &&
               bytes[cursor] != 0x3fu &&
               bytes[cursor] != 0x23u) {
            ++cursor;
        }
    }
    path_end = cursor;

    if (cursor < byte_count &&
        bytes[cursor] == 0x3fu) {
        query_start = ++cursor;
        while (cursor < byte_count &&
               bytes[cursor] != 0x23u) {
            ++cursor;
        }
        query_end = cursor;
    }

    if (cursor < byte_count &&
        bytes[cursor] == 0x23u) {
        fragment_start = cursor + 1u;
        cursor = byte_count;
    }

    if (cursor != byte_count) {
        return RIVET_ERR_UNSUPPORTED;
    }

    candidate.bytes = bytes;
    candidate.byte_count = byte_count;
    candidate.scheme.offset = 0u;
    candidate.scheme.length = scheme_end;
    candidate.host.offset = host_start;
    candidate.host.length = host_end - host_start;
    candidate.path.offset = path_start;
    candidate.path.length = path_end - path_start;
    candidate.query.offset = query_start;
    candidate.query.length =
        query_end >= query_start ? query_end - query_start : 0u;
    candidate.fragment.offset = fragment_start;
    candidate.fragment.length =
        fragment_start != 0u ? byte_count - fragment_start : 0u;
    candidate.port = port;
    candidate.has_port = has_port;
    candidate.secure = secure;

    *url = candidate;
    return RIVET_OK;
}

rivet_result rivet_utf8_decode(
    const unsigned char *bytes,
    size_t byte_count,
    unsigned int *codepoints,
    size_t capacity,
    size_t *codepoint_count
)
{
    size_t offset = 0u;
    size_t count = 0u;

    if ((byte_count != 0u && bytes == NULL) ||
        codepoint_count == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    while (offset < byte_count) {
        unsigned int codepoint;
        size_t used;
        rivet_result result = doc_utf8_one(
            bytes,
            byte_count,
            offset,
            &codepoint,
            &used
        );

        if (result != RIVET_OK) {
            return result;
        }

        if (count == (size_t)-1) {
            return RIVET_ERR_CAPACITY;
        }
        ++count;
        offset += used;
    }

    if (count > capacity ||
        (count != 0u && codepoints == NULL)) {
        return RIVET_ERR_CAPACITY;
    }

    offset = 0u;
    count = 0u;
    while (offset < byte_count) {
        unsigned int codepoint;
        size_t used;
        rivet_result result = doc_utf8_one(
            bytes,
            byte_count,
            offset,
            &codepoint,
            &used
        );

        if (result != RIVET_OK) {
            return result;
        }
        codepoints[count++] = codepoint;
        offset += used;
    }

    *codepoint_count = count;
    return RIVET_OK;
}
