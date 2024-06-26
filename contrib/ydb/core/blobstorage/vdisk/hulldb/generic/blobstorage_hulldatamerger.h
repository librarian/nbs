#pragma once

#include "defs.h"
#include <contrib/ydb/core/blobstorage/vdisk/hulldb/base/blobstorage_hulldefs.h>
#include <contrib/ydb/core/blobstorage/vdisk/hulldb/base/blobstorage_blob.h>
#include <contrib/ydb/core/blobstorage/vdisk/huge/blobstorage_hullhugedefs.h>
#include <util/generic/noncopyable.h>

namespace NKikimr {

    ////////////////////////////////////////////////////////////////////////////
    // TDataMerger
    ////////////////////////////////////////////////////////////////////////////
    class TDataMerger : TNonCopyable {
    private:
        TDiskBlobMerger DiskBlobMerger;
        NHuge::TBlobMerger HugeBlobMerger;

    public:
        TDataMerger()
            : DiskBlobMerger()
            , HugeBlobMerger()
        {}

        bool Empty() const {
            return DiskBlobMerger.Empty() && HugeBlobMerger.Empty();
        }

        void Clear() {
            DiskBlobMerger.Clear();
            HugeBlobMerger.Clear();
        }

        void Swap(TDataMerger &m) {
            DiskBlobMerger.Swap(m.DiskBlobMerger);
            HugeBlobMerger.Swap(m.HugeBlobMerger);
        }

        TBlobType::EType GetType() const {
            Y_DEBUG_ABORT_UNLESS(HugeBlobMerger.Empty() || DiskBlobMerger.Empty());
            if (!HugeBlobMerger.Empty()) {
                return HugeBlobMerger.GetBlobType();
            } else {
                // both for !DiskBlobMerger.Empty() and DiskBlobMerger.Empty()
                return TBlobType::DiskBlob;
            }
        }

        ui32 GetInplacedSize() const {
            Y_DEBUG_ABORT_UNLESS(HugeBlobMerger.Empty() || DiskBlobMerger.Empty());
            return HugeBlobMerger.Empty() ? DiskBlobMerger.GetDiskBlob().GetSize() : 0;
        }

        void AddHugeBlob(const TDiskPart *begin, const TDiskPart *end, const NMatrix::TVectorType &parts,
                ui64 circaLsn) {
            Y_DEBUG_ABORT_UNLESS(DiskBlobMerger.Empty());
            HugeBlobMerger.Add(begin, end, parts, circaLsn);
        }

        void AddBlob(const TDiskBlob &addBlob) {
            Y_DEBUG_ABORT_UNLESS(HugeBlobMerger.Empty());
            DiskBlobMerger.Add(addBlob);
        }

        void AddPart(const TDiskBlob& source, const TDiskBlob::TPartIterator& iter) {
            Y_DEBUG_ABORT_UNLESS(HugeBlobMerger.Empty());
            DiskBlobMerger.AddPart(source, iter);
        }

        void SetEmptyFromAnotherMerger(const TDataMerger *dataMerger) {
            DiskBlobMerger.Clear();
            HugeBlobMerger.SetEmptyFromAnotherMerger(&dataMerger->HugeBlobMerger);
        }

        bool HasSmallBlobs() const {
            return !DiskBlobMerger.Empty();
        }

        ui32 GetDiskBlobRawSize() const {
            Y_DEBUG_ABORT_UNLESS(!DiskBlobMerger.Empty());
            return DiskBlobMerger.GetDiskBlob().GetSize();
        }

        const TDiskBlobMerger &GetDiskBlobMerger() const {
            return DiskBlobMerger;
        }

        const NHuge::TBlobMerger &GetHugeBlobMerger() const {
            return HugeBlobMerger;
        }

        TString ToString() const {
            TStringStream str;
            str << "{DiskBlobMerger# " << DiskBlobMerger.ToString();
            str << " HugeBlobMerger# " << HugeBlobMerger.ToString() << "}";
            return str.Str();
        }

    };

} // NKikimr
