/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */
#include "hermes/BCGen/HBC/BytecodeDataProvider.h"
#include "hermes/BCGen/HBC/BytecodeFileFormat.h"
#include "hermes/Support/ErrorHandling.h"
#include "hermes/Support/OSCompat.h"

#include "llvm/Support/MathExtras.h"

namespace hermes {
namespace hbc {

namespace {
/// Returns if aref points to valid bytecode and specifies why it may not
/// in errorMessage (if supplied).
static bool sanityCheck(
    llvm::ArrayRef<uint8_t> aref,
    BytecodeForm form,
    std::string *errorMessage) {
  if (aref.size() < sizeof(hbc::BytecodeFileHeader)) {
    if (errorMessage) {
      *errorMessage = "Buffer too small";
    }
    return false;
  }

  const auto *header =
      reinterpret_cast<const hbc::BytecodeFileHeader *>(aref.data());

  auto magic = (form == BytecodeForm::Delta ? DELTA_MAGIC : MAGIC);
  if (header->magic != magic) {
    if (errorMessage) {
      *errorMessage = "Incorrect magic number";
    }
    return false;
  }
  if (header->version != hbc::BYTECODE_VERSION) {
    if (errorMessage) {
      llvm::raw_string_ostream errs(*errorMessage);
      errs << "Wrong bytecode version. Expected " << hbc::BYTECODE_VERSION
           << " but got " << header->version;
    }
    return false;
  }
  if (header->functionCount == 0) {
    if (errorMessage) {
      *errorMessage = "Bytecode does not contain any functions";
    }
    return false;
  }
  return true;
}

/// Assert that \p buf has the proper alignment for T, and then cast it to a
/// pointer to T. \return the pointer to T.
template <typename T>
const T *alignCheckCast(const uint8_t *buf) {
  // We pad the offset of each data structure by 4 bytes, hence we cannot
  // support casting to any data structure that requires more than 4 bytes
  // alignment, which may lead to undefined behavior.
  static_assert(alignof(T) <= 4, "Cannot handle the alignment");
  assert(
      (llvm::alignAddr(buf, alignof(T)) == (uintptr_t)buf) &&
      "buf is not properly aligned");
  return reinterpret_cast<const T *>(buf);
}

/// Variant of alignCheckCast() for non-const pointers.
template <typename T>
T *alignCheckCast(uint8_t *buf) {
  static_assert(alignof(T) <= 4, "Cannot handle the alignment");
  assert(
      (llvm::alignAddr(buf, alignof(T)) == (uintptr_t)buf) &&
      "buf is not properly aligned");
  return reinterpret_cast<T *>(buf);
}

/// Cast the pointer at \p buf to type T, increment \p buf by
/// the size of T.
template <typename T>
const T *castData(const uint8_t *&buf) {
  auto ret = alignCheckCast<T>(buf);
  buf += sizeof(T);
  return ret;
}

/// Variant of castData() for non-const pointers.
template <typename T>
T *castData(uint8_t *&buf) {
  auto ret = alignCheckCast<T>(buf);
  buf += sizeof(T);
  return ret;
}

/// Cast the pointer at \p buf to an array of type T, with \p size.
/// Increment \p buf by the total size of the array.
template <typename T>
llvm::ArrayRef<T> castArrayRef(const uint8_t *&buf, size_t size) {
  auto ptr = alignCheckCast<T>(buf);
  buf += size * sizeof(T);
  return {ptr, size};
}

/// Variant of castArrayRef() for non-const pointers.
template <typename T>
llvm::MutableArrayRef<T> castArrayRef(uint8_t *&buf, size_t size) {
  auto ptr = alignCheckCast<T>(buf);
  buf += size * sizeof(T);
  return {ptr, size};
}

/// Align \p buf with the \p alignment.
/// \p buf is passed by pointer reference and will be modified.
void align(const uint8_t *&buf, uint32_t alignment = 4) {
  buf = (const uint8_t *)llvm::alignAddr(buf, alignment);
}

/// Variant of align() for non-const pointers.
void align(uint8_t *&buf, uint32_t alignment = 4) {
  buf = (uint8_t *)llvm::alignAddr(buf, alignment);
}

} // namespace

template <bool Mutable>
bool BytecodeFileFields<Mutable>::populateFromBuffer(
    Array<uint8_t> buffer,
    std::string *outError,
    BytecodeForm form) {
  if (!sanityCheck(buffer, form, outError)) {
    return false;
  }

  // Helper type which populates a BytecodeFileFields. This is nested inside the
  // function so we can leverage BytecodeFileFields template types.
  struct BytecodeFileFieldsPopulator {
    /// The fields being populated.
    BytecodeFileFields &f;

    /// Current buffer position.
    Pointer<uint8_t> buf;

    /// A pointer to the bytecode file header.
    const BytecodeFileHeader *h;

    BytecodeFileFieldsPopulator(
        BytecodeFileFields &fields,
        Pointer<uint8_t> buffer)
        : f(fields), buf(buffer) {
      f.header = castData<BytecodeFileHeader>(buf);
      h = f.header;
    }

    void visitFunctionHeaders() {
      align(buf);
      f.functionHeaders = castArrayRef<SmallFuncHeader>(buf, h->functionCount);
    }

    void visitStringKinds() {
      align(buf);
      f.stringKinds = castArrayRef<StringKind::Entry>(buf, h->stringKindCount);
    }

    void visitIdentifierTranslations() {
      align(buf);
      f.identifierTranslations =
          castArrayRef<uint32_t>(buf, h->identifierCount);
    }

    void visitSmallStringTable() {
      align(buf);
      f.stringTableEntries =
          castArrayRef<SmallStringTableEntry>(buf, h->stringCount);
    }

    void visitOverflowStringTable() {
      align(buf);
      f.stringTableOverflowEntries =
          castArrayRef<OverflowStringTableEntry>(buf, h->overflowStringCount);
    }

    void visitStringStorage() {
      align(buf);
      f.stringStorage = castArrayRef<char>(buf, h->stringStorageSize);
    }
    void visitArrayBuffer() {
      align(buf);
      f.arrayBuffer = castArrayRef<unsigned char>(buf, h->arrayBufferSize);
    }
    void visitObjectKeyBuffer() {
      align(buf);
      f.objKeyBuffer = castArrayRef<unsigned char>(buf, h->objKeyBufferSize);
    }
    void visitObjectValueBuffer() {
      align(buf);
      f.objValueBuffer =
          castArrayRef<unsigned char>(buf, h->objValueBufferSize);
    }
    void visitRegExpTable() {
      align(buf);
      f.regExpTable = castArrayRef<RegExpTableEntry>(buf, h->regExpCount);
    }
    void visitRegExpStorage() {
      align(buf);
      f.regExpStorage = castArrayRef<unsigned char>(buf, h->regExpStorageSize);
    }
    void visitCJSModuleTable() {
      align(buf);
      if (h->options.cjsModulesStaticallyResolved) {
        // Modules have been statically resolved.
        f.cjsModuleTableStatic = castArrayRef<uint32_t>(buf, h->cjsModuleCount);
      } else {
        // Modules are not resolved, use the filename -> function ID mapping.
        f.cjsModuleTable =
            castArrayRef<std::pair<uint32_t, uint32_t>>(buf, h->cjsModuleCount);
      }
    }
  };

  BytecodeFileFieldsPopulator populator{*this, buffer.data()};
  visitBytecodeSegmentsInOrder(populator);
  return true;
}

// Explicit instantiations of BytecodeFileFields.
template struct BytecodeFileFields<false>;
template struct BytecodeFileFields<true>;

int32_t BCProviderBase::findCatchTargetOffset(
    uint32_t functionID,
    uint32_t exceptionOffset) const {
  auto exceptions = getExceptionTable(functionID);
  for (unsigned i = 0, e = exceptions.size(); i < e; ++i) {
    if (exceptions[i].start <= exceptionOffset &&
        exceptionOffset < exceptions[i].end) {
      return exceptions[i].target;
    }
  }
  // No handler is found.
  return -1;
}

uint32_t BCProviderBase::getVirtualOffsetForFunction(
    uint32_t functionID) const {
  assert(functionID < functionCount_ && "Invalid functionID");
  uint32_t virtualOffset = 0;
  for (uint32_t i = 0; i < functionID; ++i) {
    virtualOffset += getFunctionHeader(i).bytecodeSizeInBytes();
  }
  return virtualOffset;
}

llvm::Optional<SourceMapTextLocation> BCProviderBase::getLocationForAddress(
    uint32_t funcId,
    uint32_t offsetInFunction) const {
  auto *funcDebugOffsets = getDebugOffsets(funcId);
  if (funcDebugOffsets != nullptr &&
      funcDebugOffsets->sourceLocations != hbc::DebugOffsets::NO_OFFSET) {
    const hbc::DebugInfo *debugInfo = getDebugInfo();
    assert(debugInfo != nullptr && "debugInfo is null");
    OptValue<DebugSourceLocation> locOpt = debugInfo->getLocationForAddress(
        funcDebugOffsets->sourceLocations, offsetInFunction);
    if (locOpt.hasValue()) {
      DebugSourceLocation loc = locOpt.getValue();
      std::string fileName = debugInfo->getFilenameByID(loc.filenameId);
      return SourceMapTextLocation{std::move(fileName), loc.line, loc.column};
    }
  }
  return llvm::None;
}

/// Read [data, data + size) sequentially into the OS page cache, but
/// abort ASAP if another thread sets \p abortFlag.
static void
warmup(const uint8_t *data, uint32_t size, std::atomic<bool> *abortFlag) {
  // The readahead/madvise syscalls are not always enough, so actually read
  // a byte from every page in the range.
  const uint32_t PS = oscompat::page_size();
  // Check abort flag every this many bytes, to ensure timely termination.
  const uint32_t kAbortCheckInterval = 64 * PS;
  uint32_t nextAbortCheckPoint = kAbortCheckInterval;
  for (uint32_t i = 0; i < size; i += PS) {
    // volatile to prevent the compiler from optimizing the read away.
    (void)(((volatile const uint8_t *)data)[i]);
    if (i >= nextAbortCheckPoint) {
      if (abortFlag->load(std::memory_order_acquire)) {
        return;
      }
      nextAbortCheckPoint += kAbortCheckInterval;
    }
  }
}

void BCProviderFromBuffer::stopWarmup() {
  if (warmupThread_) {
    warmupAbortFlag_.store(true, std::memory_order_release);
    warmupThread_->join();
    warmupThread_.reset();
  }
}

void BCProviderFromBuffer::startWarmup(uint8_t percent) {
  if (!warmupThread_) {
    uint32_t warmupSize = buffer_->size();
    assert(percent <= 100);
    if (percent < 100) {
      warmupSize = (uint64_t)warmupSize * percent / 100;
    }
    if (warmupSize > 0) {
      warmupThread_ =
          std::thread(warmup, buffer_->data(), warmupSize, &warmupAbortFlag_);
    }
  }
}

namespace {

/// Cast a pointer of any type to a uint8_t pointer.
template <typename T>
constexpr uint8_t *rawptr_cast(T *p) {
  return const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(p));
}

/// Align \p *ptr down to the start of the page it is pointing in to, and
/// simultaneously adjust \p *byteLen up by the amount the ptr was shifted down
/// by.
inline void pageAlignDown(uint8_t **ptr, size_t *byteLen) {
  const auto PS = oscompat::page_size();

  auto orig = *ptr;
  *ptr = reinterpret_cast<uint8_t *>(llvm::alignAddr(*ptr + 1, PS) - PS);
  byteLen += orig - *ptr;
}

#ifndef NDEBUG

/// Returns the total size of all array contents in bytes.
constexpr size_t totalByteSize() {
  return 0;
}

template <typename T, typename... Ts>
constexpr size_t totalByteSize(
    llvm::ArrayRef<T> arr,
    llvm::ArrayRef<Ts>... rest) {
  return sizeof(arr[0]) * arr.size() + totalByteSize(rest...);
}

#endif

} // namespace

void BCProviderFromBuffer::madvise(oscompat::MAdvice advice) {
  (void)oscompat::vm_madvise(
      rawptr_cast(buffer_->data()), buffer_->size(), advice);
}

#define ASSERT_BOUNDED(LO, ARRAY, HI)                                       \
  assert(                                                                   \
      LO <= rawptr_cast(ARRAY.begin()) && rawptr_cast(ARRAY.end()) <= HI && \
      #ARRAY " not fully contained.")

#define ASSERT_TOTAL_ARRAY_LEN(LEN, ...) \
  assert(LEN == totalByteSize(__VA_ARGS__) && "Mismatched length of region")

void BCProviderFromBuffer::adviseStringTableSequential() {
  llvm::ArrayRef<SmallStringTableEntry> smallStringTableEntries{
      stringTableEntries_, stringCount_};

  auto *start = rawptr_cast(stringKinds_.begin());
  auto *end = rawptr_cast(stringStorage_.begin());
  size_t adviceLength = end - start;

  ASSERT_BOUNDED(start, stringKinds_, end);
  ASSERT_BOUNDED(start, identifierTranslations_, end);
  ASSERT_BOUNDED(start, smallStringTableEntries, end);
  ASSERT_BOUNDED(start, overflowStringTableEntries_, end);

  ASSERT_TOTAL_ARRAY_LEN(
      adviceLength,
      stringKinds_,
      identifierTranslations_,
      smallStringTableEntries,
      overflowStringTableEntries_);

  pageAlignDown(&start, &adviceLength);
  oscompat::vm_madvise(start, adviceLength, oscompat::MAdvice::Sequential);
}

void BCProviderFromBuffer::adviseStringTableRandom() {
  llvm::ArrayRef<SmallStringTableEntry> smallStringTableEntries{
      stringTableEntries_, stringCount_};

  // We only advise the small string table entries, overflow string table
  // entries and storage.  We do not give advice about the identifier
  // translations or string kinds because they are not referred to after
  // initialisation.

  auto *tableStart = rawptr_cast(stringTableEntries_);
  auto *tableEnd = rawptr_cast(overflowStringTableEntries_.end());
  size_t tableLength = tableEnd - tableStart;

  auto *storageStart = rawptr_cast(stringStorage_.begin());
  size_t storageLength = stringStorage_.size();

  ASSERT_BOUNDED(tableStart, smallStringTableEntries, tableEnd);
  ASSERT_BOUNDED(tableStart, overflowStringTableEntries_, tableEnd);

  ASSERT_TOTAL_ARRAY_LEN(
      tableLength, smallStringTableEntries, overflowStringTableEntries_);

  pageAlignDown(&tableStart, &tableLength);
  pageAlignDown(&storageStart, &storageLength);
  oscompat::vm_madvise(tableStart, tableLength, oscompat::MAdvice::Random);
  oscompat::vm_madvise(storageStart, storageLength, oscompat::MAdvice::Random);
}

void BCProviderFromBuffer::willNeedStringTable() {
  llvm::ArrayRef<SmallStringTableEntry> smallStringTableEntries{
      stringTableEntries_, stringCount_};

  auto *start = rawptr_cast(stringKinds_.begin());
  auto *end = rawptr_cast(overflowStringTableEntries_.end());
  size_t prefetchLength = end - start;

  ASSERT_BOUNDED(start, stringKinds_, end);
  ASSERT_BOUNDED(start, identifierTranslations_, end);
  ASSERT_BOUNDED(start, smallStringTableEntries, end);
  ASSERT_BOUNDED(start, overflowStringTableEntries_, end);

  ASSERT_TOTAL_ARRAY_LEN(
      prefetchLength,
      stringKinds_,
      identifierTranslations_,
      smallStringTableEntries,
      overflowStringTableEntries_);

  pageAlignDown(&start, &prefetchLength);
  oscompat::vm_prefetch(start, prefetchLength);
}

#undef ASSERT_BOUNDED
#undef ASSERT_TOTAL_ARRAY_LEN

void BCProviderFromBuffer::startPageAccessTracker() {
  auto size = buffer_->size();
  if (!tracker_) {
    tracker_ =
        PageAccessTracker::create(const_cast<uint8_t *>(bufferPtr_), size);
  }
}

BCProviderFromBuffer::BCProviderFromBuffer(std::unique_ptr<const Buffer> buffer)
    : buffer_(std::move(buffer)), bufferPtr_(buffer_->data()) {
  ConstBytecodeFileFields fields;
  if (!fields.populateFromBuffer({bufferPtr_, buffer_->size()}, &errstr_)) {
    return;
  }
  const auto *fileHeader = fields.header;
  options_ = fileHeader->options;
  functionCount_ = fileHeader->functionCount;
  globalFunctionIndex_ = fileHeader->globalCodeIndex;
  debugInfoOffset_ = fileHeader->debugInfoOffset;
  functionHeaders_ = fields.functionHeaders.data();
  stringKinds_ = fields.stringKinds;
  identifierTranslations_ = fields.identifierTranslations;
  stringCount_ = fileHeader->stringCount;
  stringTableEntries_ = fields.stringTableEntries.data();
  overflowStringTableEntries_ = fields.stringTableOverflowEntries;
  stringStorage_ = fields.stringStorage;
  arrayBuffer_ = fields.arrayBuffer;
  objKeyBuffer_ = fields.objKeyBuffer;
  objValueBuffer_ = fields.objValueBuffer;
  regExpTable_ = fields.regExpTable;
  regExpStorage_ = fields.regExpStorage;
  cjsModuleOffset_ = fileHeader->cjsModuleOffset;
  cjsModuleTable_ = fields.cjsModuleTable;
  cjsModuleTableStatic_ = fields.cjsModuleTableStatic;
}

llvm::ArrayRef<uint8_t> BCProviderFromBuffer::getEpilogue() const {
  return BCProviderFromBuffer::getEpilogueFromBytecode(
      llvm::ArrayRef<uint8_t>(bufferPtr_, buffer_->size()));
}

SHA1 BCProviderFromBuffer::getSourceHash() const {
  return BCProviderFromBuffer::getSourceHashFromBytecode(
      llvm::ArrayRef<uint8_t>(bufferPtr_, buffer_->size()));
}

llvm::ArrayRef<uint8_t> BCProviderFromBuffer::getEpilogueFromBytecode(
    llvm::ArrayRef<uint8_t> buffer) {
  const uint8_t *p = buffer.data();
  const auto *fileHeader = castData<hbc::BytecodeFileHeader>(p);
  const auto *begin = buffer.data() + fileHeader->fileLength;
  const auto *end = buffer.data() + buffer.size();
  return llvm::ArrayRef<uint8_t>(begin, end);
}

SHA1 BCProviderFromBuffer::getSourceHashFromBytecode(
    llvm::ArrayRef<uint8_t> buffer) {
  SHA1 hash;
  const uint8_t *p = buffer.data();
  const auto *fileHeader = castData<hbc::BytecodeFileHeader>(p);
  std::copy(
      fileHeader->sourceHash,
      fileHeader->sourceHash + SHA1_NUM_BYTES,
      hash.begin());
  return hash;
}

void BCProviderFromBuffer::createDebugInfo() {
  const auto *buf = bufferPtr_ + debugInfoOffset_;
  const auto *header = castData<hbc::DebugInfoHeader>(buf);

  auto filenameTable =
      castArrayRef<StringTableEntry>(buf, header->filenameCount);
  auto filenameStorage = castArrayRef<char>(buf, header->filenameStorageSize);

  hbc::DebugInfo::DebugFileRegionList files;
  for (unsigned i = 0; i < header->fileRegionCount; i++) {
    const auto *region = castData<hbc::DebugFileRegion>(buf);
    files.push_back(*region);
  }
  debugInfo_ = new hbc::DebugInfo(
      filenameTable,
      filenameStorage,
      std::move(files),
      header->lexicalDataOffset,
      hbc::StreamVector<uint8_t>{buf, header->debugDataSize});
}

std::pair<
    llvm::ArrayRef<hbc::HBCExceptionHandlerInfo>,
    const hbc::DebugOffsets *>
BCProviderFromBuffer::getExceptionTableAndDebugOffsets(
    uint32_t functionID) const {
  const auto &header = functionHeaders_[functionID];
  const auto *buf = bufferPtr_;

  // Get the correct offset for function info depending on overflow flag. Skip
  // large header if any (we don't need to parse it, since we're only using
  // flags below, which are also valid for overflowed small headers).
  if (header.flags.overflowed) {
    buf += header.getLargeHeaderOffset();
    buf += sizeof(hbc::FunctionHeader);
  } else {
    buf += header.infoOffset;
  }

  // Deserialize exception table.
  llvm::ArrayRef<hbc::HBCExceptionHandlerInfo> exceptionTable{};
  if (header.flags.hasExceptionHandler) {
    align(buf);
    const auto *exceptionHeader =
        castData<hbc::ExceptionHandlerTableHeader>(buf);
    exceptionTable =
        castArrayRef<hbc::HBCExceptionHandlerInfo>(buf, exceptionHeader->count);
  }

  // Deserialize debug offsets.
  const hbc::DebugOffsets *debugOffsets = nullptr;
  if (header.flags.hasDebugInfo) {
    align(buf);
    debugOffsets = castData<hbc::DebugOffsets>(buf);
  }
  return {exceptionTable, debugOffsets};
}

namespace {
void prefetchRegion(const uint8_t *p, size_t sz) {
  // Extend start of region down to a page boundary. The region is still inside
  // the file since we assert below that the file starts on a page boundary.
  auto PS = oscompat::page_size();
  auto roundDownDelta = reinterpret_cast<uintptr_t>(p) & (PS - 1);
  oscompat::vm_prefetch(
      const_cast<uint8_t *>(p - roundDownDelta), sz + roundDownDelta);
}
} // namespace

void BCProviderFromBuffer::prefetch(llvm::ArrayRef<uint8_t> aref) {
  // We require file start be page-aligned so we can safely round down to page
  // size in prefetchRegion.
  assert(
      reinterpret_cast<uintptr_t>(aref.data()) % oscompat::page_size() == 0 &&
      "Precondition: pointer is page-aligned.");
  ConstBytecodeFileFields fields;
  std::string errstr;
  if (!fields.populateFromBuffer(aref, &errstr)) {
#ifndef NDEBUG
    hermes_fatal(errstr);
#else
    return;
#endif
  }
  const hbc::BytecodeFileHeader *fileHeader = fields.header;

  // String table.
  auto stringCount = fileHeader->stringCount;
  const hbc::SmallStringTableEntry *stringTableEntries =
      fields.stringTableEntries.data();
  prefetchRegion(
      reinterpret_cast<const uint8_t *>(stringTableEntries),
      stringCount * sizeof(*stringTableEntries));

  // Global function bytecode.
  auto globalFunctionIndex = fileHeader->globalCodeIndex;
  auto functionHeaders = fields.functionHeaders.data();
  const SmallFuncHeader &globalSmall = functionHeaders[globalFunctionIndex];
  RuntimeFunctionHeader global = globalSmall.flags.overflowed
      ? RuntimeFunctionHeader(reinterpret_cast<const hbc::FunctionHeader *>(
            aref.data() + globalSmall.getLargeHeaderOffset()))
      : RuntimeFunctionHeader(&globalSmall);
  prefetchRegion(aref.data() + global.offset(), global.bytecodeSizeInBytes());
}

bool BCProviderFromBuffer::bytecodeStreamSanityCheck(
    llvm::ArrayRef<uint8_t> aref,
    std::string *errorMessage) {
  return sanityCheck(aref, BytecodeForm::Execution, errorMessage);
}

} // namespace hbc
} // namespace hermes
