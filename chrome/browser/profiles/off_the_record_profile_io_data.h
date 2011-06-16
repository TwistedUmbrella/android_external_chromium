// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_OFF_THE_RECORD_PROFILE_IO_DATA_H_
#define CHROME_BROWSER_PROFILES_OFF_THE_RECORD_PROFILE_IO_DATA_H_
#pragma once

#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/profiles/profile_io_data.h"

class ChromeURLRequestContext;
class ChromeURLRequestContextGetter;
class IOThread;
class Profile;

// OffTheRecordProfile owns a OffTheRecordProfileIOData::Handle, which holds a
// reference to the OffTheRecordProfileIOData. OffTheRecordProfileIOData is
// intended to own all the objects owned by OffTheRecordProfile which live on
// the IO thread, such as, but not limited to, network objects like
// CookieMonster, HttpTransactionFactory, etc. OffTheRecordProfileIOData is
// owned by the OffTheRecordProfile and OffTheRecordProfileIOData's
// ChromeURLRequestContexts. When all of them go away, then ProfileIOData will
// be deleted. Note that the OffTheRecordProfileIOData will typically outlive
// the Profile it is "owned" by, so it's important for OffTheRecordProfileIOData
// not to hold any references to the Profile beyond what's used by LazyParams
// (which should be deleted after lazy initialization).

class OffTheRecordProfileIOData : public ProfileIOData {
 public:
  class Handle {
   public:
    explicit Handle(Profile* profile);
    ~Handle();

    scoped_refptr<ChromeURLRequestContextGetter>
        GetMainRequestContextGetter() const;
    scoped_refptr<ChromeURLRequestContextGetter>
        GetExtensionsRequestContextGetter() const;

   private:
    // Lazily initialize ProfileParams. We do this on the calls to
    // Get*RequestContextGetter(), so we only initialize ProfileParams right
    // before posting a task to the IO thread to start using them. This prevents
    // objects that are supposed to be deleted on the IO thread, but are created
    // on the UI thread from being unnecessarily initialized.
    void LazyInitialize() const;

    // Ordering is important here. Do not reorder unless you know what you're
    // doing. We need to release |io_data_| *before* the getters, because we
    // want to make sure that the last reference for |io_data_| is on the IO
    // thread. The getters will be deleted on the IO thread, so they will
    // release their refs to their contexts, which will release the last refs to
    // the ProfileIOData on the IO thread.
    mutable scoped_refptr<ChromeURLRequestContextGetter>
        main_request_context_getter_;
    mutable scoped_refptr<ChromeURLRequestContextGetter>
        extensions_request_context_getter_;
    const scoped_refptr<OffTheRecordProfileIOData> io_data_;

    Profile* const profile_;

    mutable bool initialized_;

    DISALLOW_COPY_AND_ASSIGN(Handle);
  };

 private:
  friend class base::RefCountedThreadSafe<OffTheRecordProfileIOData>;

  struct LazyParams {
    LazyParams();
    ~LazyParams();

    IOThread* io_thread;
    ProfileParams profile_params;
  };

  OffTheRecordProfileIOData();
  ~OffTheRecordProfileIOData();

  // Lazily initializes ProfileIOData.
  virtual void LazyInitializeInternal() const;
  virtual scoped_refptr<ChromeURLRequestContext>
      AcquireMainRequestContext() const;
  virtual scoped_refptr<ChromeURLRequestContext>
      AcquireMediaRequestContext() const;
  virtual scoped_refptr<ChromeURLRequestContext>
      AcquireExtensionsRequestContext() const;

  // Lazy initialization params.
  mutable scoped_ptr<LazyParams> lazy_params_;

  mutable bool initialized_;
  mutable scoped_refptr<RequestContext> main_request_context_;
  // NOTE: |media_request_context_| just points to the same context that
  // |main_request_context_| points to.
  mutable scoped_refptr<RequestContext> media_request_context_;
  mutable scoped_refptr<RequestContext> extensions_request_context_;

  mutable scoped_ptr<net::NetworkDelegate> network_delegate_;
  mutable scoped_ptr<net::DnsCertProvenanceChecker> dns_cert_checker_;
  mutable scoped_ptr<net::HttpTransactionFactory> main_http_factory_;

  DISALLOW_COPY_AND_ASSIGN(OffTheRecordProfileIOData);
};

#endif  // CHROME_BROWSER_PROFILES_OFF_THE_RECORD_PROFILE_IO_DATA_H_