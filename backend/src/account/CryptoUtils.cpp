#include "CryptoUtils.hpp"
#include "utils/Logger.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <vector>
#include <cstring>

namespace SmartMail {

// --- PBKDF2 密钥派生 ---

std::string CryptoUtils::deriveKey(const std::string& masterPassword,
                                    const std::string& salt) {
    constexpr int kKeyLength = 32;   // AES-256
    constexpr int kIterations = 100000;

    std::vector<unsigned char> derivedKey(kKeyLength);

    int rc = PKCS5_PBKDF2_HMAC(
        masterPassword.c_str(),
        static_cast<int>(masterPassword.size()),
        reinterpret_cast<const unsigned char*>(salt.data()),
        static_cast<int>(salt.size()),
        kIterations,
        EVP_sha256(),
        kKeyLength,
        derivedKey.data());

    if (rc != 1) {
        LOG_ERROR("PBKDF2 key derivation failed");
        return "";
    }

    return std::string(reinterpret_cast<char*>(derivedKey.data()), kKeyLength);
}

// --- AES-256-CBC 加密 ---

std::string CryptoUtils::encrypt(const std::string& plaintext,
                                  const std::string& key) {
    if (key.size() != 32 || plaintext.empty()) {
        LOG_ERROR("encrypt: key must be 32 bytes and plaintext non-empty");
        return "";
    }

    ERR_clear_error();

    // 生成随机 IV (16 bytes)
    std::vector<unsigned char> iv(16);
    if (RAND_bytes(iv.data(), 16) != 1) {
        LOG_ERROR("Failed to generate random IV");
        return "";
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        LOG_ERROR("Failed to create cipher context");
        return "";
    }

    int rc = EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                                reinterpret_cast<const unsigned char*>(key.data()),
                                iv.data());
    if (rc != 1) {
        LOG_ERROR("EVP_EncryptInit_ex failed");
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    // 分配输出缓冲区 (明文长度 + 一个 block + 16字节IV)
    std::vector<unsigned char> outBuf(plaintext.size() + EVP_CIPHER_CTX_block_size(ctx) + 16);
    int outLen = 0;
    int totalLen = 0;

    // 写入 IV (前16字节)
    std::memcpy(outBuf.data(), iv.data(), 16);
    totalLen = 16;

    rc = EVP_EncryptUpdate(ctx, outBuf.data() + totalLen, &outLen,
                           reinterpret_cast<const unsigned char*>(plaintext.data()),
                           static_cast<int>(plaintext.size()));
    if (rc != 1) {
        LOG_ERROR("EVP_EncryptUpdate failed");
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    totalLen += outLen;

    rc = EVP_EncryptFinal_ex(ctx, outBuf.data() + totalLen, &outLen);
    if (rc != 1) {
        LOG_ERROR("EVP_EncryptFinal_ex failed");
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    totalLen += outLen;

    EVP_CIPHER_CTX_free(ctx);

    return base64Encode(std::string(reinterpret_cast<char*>(outBuf.data()), totalLen));
}

// --- AES-256-CBC 解密 ---

std::string CryptoUtils::decrypt(const std::string& ciphertext,
                                  const std::string& key) {
    if (key.size() != 32 || ciphertext.empty()) {
        LOG_ERROR("decrypt: key must be 32 bytes and ciphertext non-empty");
        return "";
    }

    ERR_clear_error();

    // Base64 解码
    std::string raw = base64Decode(ciphertext);
    if (raw.size() < 17) { // 至少 16 byte IV + 1 byte 数据
        LOG_ERROR("decrypt: data too short after base64 decode");
        return "";
    }

    // 提取 IV (前 16 字节)
    const unsigned char* iv = reinterpret_cast<const unsigned char*>(raw.data());
    const unsigned char* encData = iv + 16;
    int encLen = static_cast<int>(raw.size() - 16);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        LOG_ERROR("Failed to create cipher context");
        return "";
    }

    int rc = EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                                reinterpret_cast<const unsigned char*>(key.data()),
                                iv);
    if (rc != 1) {
        LOG_ERROR("EVP_DecryptInit_ex failed");
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    std::vector<unsigned char> outBuf(encLen + EVP_CIPHER_CTX_block_size(ctx));
    int outLen = 0;
    int totalLen = 0;

    rc = EVP_DecryptUpdate(ctx, outBuf.data(), &outLen, encData, encLen);
    if (rc != 1) {
        LOG_ERROR("EVP_DecryptUpdate failed");
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    totalLen += outLen;

    rc = EVP_DecryptFinal_ex(ctx, outBuf.data() + totalLen, &outLen);
    if (rc != 1) {
        LOG_ERROR("EVP_DecryptFinal_ex failed (wrong key or corrupted data)");
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    totalLen += outLen;

    EVP_CIPHER_CTX_free(ctx);

    return std::string(reinterpret_cast<char*>(outBuf.data()), totalLen);
}

// --- Base64 编解码 ---

std::string CryptoUtils::base64Encode(const std::string& data) {
    if (data.empty()) return "";

    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bio);

    BIO_write(b64, data.data(), static_cast<int>(data.size()));
    BIO_flush(b64);

    BUF_MEM* buf = nullptr;
    BIO_get_mem_ptr(b64, &buf);

    std::string result(buf->data, buf->length);
    BIO_free_all(b64);
    return result;
}

std::string CryptoUtils::base64Decode(const std::string& encoded) {
    if (encoded.empty()) return "";

    // 计算解码缓冲区大小
    int decodedLen = static_cast<int>(encoded.size()) * 3 / 4 + 4;
    std::vector<char> outBuf(decodedLen);

    BIO* bio = BIO_new_mem_buf(encoded.data(), static_cast<int>(encoded.size()));
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);

    int len = BIO_read(bio, outBuf.data(), static_cast<int>(outBuf.size()));
    BIO_free_all(bio);

    if (len <= 0) {
        LOG_ERROR("Base64 decode failed");
        return "";
    }
    return std::string(outBuf.data(), len);
}

// --- 随机 Salt 生成 ---

std::string CryptoUtils::generateSalt(size_t length) {
    std::vector<unsigned char> salt(length);
    if (RAND_bytes(salt.data(), static_cast<int>(length)) != 1) {
        LOG_ERROR("Failed to generate random salt");
        return "";
    }
    return std::string(reinterpret_cast<char*>(salt.data()), length);
}

} // namespace SmartMail
