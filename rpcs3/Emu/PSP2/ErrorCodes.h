#pragma once

#define ERROR_CODE(code) static_cast<s32>(code)

enum SceOk : s32
{
	SCE_OK = 0,
};

enum SceError : s32
{
	SCE_ERROR_ERRNO_EPERM           = ERROR_CODE(0x80010001),
	SCE_ERROR_ERRNO_ENOENT          = ERROR_CODE(0x80010002),
	SCE_ERROR_ERRNO_ESRCH           = ERROR_CODE(0x80010003),
	SCE_ERROR_ERRNO_EINTR           = ERROR_CODE(0x80010004),
	SCE_ERROR_ERRNO_EIO             = ERROR_CODE(0x80010005),
	SCE_ERROR_ERRNO_ENXIO           = ERROR_CODE(0x80010006),
	SCE_ERROR_ERRNO_E2BIG           = ERROR_CODE(0x80010007),
	SCE_ERROR_ERRNO_ENOEXEC         = ERROR_CODE(0x80010008),
	SCE_ERROR_ERRNO_EBADF           = ERROR_CODE(0x80010009),
	SCE_ERROR_ERRNO_ECHILD          = ERROR_CODE(0x8001000A),
	SCE_ERROR_ERRNO_EAGAIN          = ERROR_CODE(0x8001000B),
	SCE_ERROR_ERRNO_ENOMEM          = ERROR_CODE(0x8001000C),
	SCE_ERROR_ERRNO_EACCES          = ERROR_CODE(0x8001000D),
	SCE_ERROR_ERRNO_EFAULT          = ERROR_CODE(0x8001000E),
	SCE_ERROR_ERRNO_ENOTBLK         = ERROR_CODE(0x8001000F),
	SCE_ERROR_ERRNO_EBUSY           = ERROR_CODE(0x80010010),
	SCE_ERROR_ERRNO_EEXIST          = ERROR_CODE(0x80010011),
	SCE_ERROR_ERRNO_EXDEV           = ERROR_CODE(0x80010012),
	SCE_ERROR_ERRNO_ENODEV          = ERROR_CODE(0x80010013),
	SCE_ERROR_ERRNO_ENOTDIR         = ERROR_CODE(0x80010014),
	SCE_ERROR_ERRNO_EISDIR          = ERROR_CODE(0x80010015),
	SCE_ERROR_ERRNO_EINVAL          = ERROR_CODE(0x80010016),
	SCE_ERROR_ERRNO_ENFILE          = ERROR_CODE(0x80010017),
	SCE_ERROR_ERRNO_EMFILE          = ERROR_CODE(0x80010018),
	SCE_ERROR_ERRNO_ENOTTY          = ERROR_CODE(0x80010019),
	SCE_ERROR_ERRNO_ETXTBSY         = ERROR_CODE(0x8001001A),
	SCE_ERROR_ERRNO_EFBIG           = ERROR_CODE(0x8001001B),
	SCE_ERROR_ERRNO_ENOSPC          = ERROR_CODE(0x8001001C),
	SCE_ERROR_ERRNO_ESPIPE          = ERROR_CODE(0x8001001D),
	SCE_ERROR_ERRNO_EROFS           = ERROR_CODE(0x8001001E),
	SCE_ERROR_ERRNO_EMLINK          = ERROR_CODE(0x8001001F),
	SCE_ERROR_ERRNO_EPIPE           = ERROR_CODE(0x80010020),
	SCE_ERROR_ERRNO_EDOM            = ERROR_CODE(0x80010021),
	SCE_ERROR_ERRNO_ERANGE          = ERROR_CODE(0x80010022),
	SCE_ERROR_ERRNO_ENOMSG          = ERROR_CODE(0x80010023),
	SCE_ERROR_ERRNO_EIDRM           = ERROR_CODE(0x80010024),
	SCE_ERROR_ERRNO_ECHRNG          = ERROR_CODE(0x80010025),
	SCE_ERROR_ERRNO_EL2NSYNC        = ERROR_CODE(0x80010026),
	SCE_ERROR_ERRNO_EL3HLT          = ERROR_CODE(0x80010027),
	SCE_ERROR_ERRNO_EL3RST          = ERROR_CODE(0x80010028),
	SCE_ERROR_ERRNO_ELNRNG          = ERROR_CODE(0x80010029),
	SCE_ERROR_ERRNO_EUNATCH         = ERROR_CODE(0x8001002A),
	SCE_ERROR_ERRNO_ENOCSI          = ERROR_CODE(0x8001002B),
	SCE_ERROR_ERRNO_EL2HLT          = ERROR_CODE(0x8001002C),
	SCE_ERROR_ERRNO_EDEADLK         = ERROR_CODE(0x8001002D),
	SCE_ERROR_ERRNO_ENOLCK          = ERROR_CODE(0x8001002E),
	SCE_ERROR_ERRNO_EFORMAT         = ERROR_CODE(0x8001002F),
	SCE_ERROR_ERRNO_EUNSUP          = ERROR_CODE(0x80010030),
	SCE_ERROR_ERRNO_EBADE           = ERROR_CODE(0x80010032),
	SCE_ERROR_ERRNO_EBADR           = ERROR_CODE(0x80010033),
	SCE_ERROR_ERRNO_EXFULL          = ERROR_CODE(0x80010034),
	SCE_ERROR_ERRNO_ENOANO          = ERROR_CODE(0x80010035),
	SCE_ERROR_ERRNO_EBADRQC         = ERROR_CODE(0x80010036),
	SCE_ERROR_ERRNO_EBADSLT         = ERROR_CODE(0x80010037),
	SCE_ERROR_ERRNO_EDEADLOCK       = ERROR_CODE(0x80010038),
	SCE_ERROR_ERRNO_EBFONT          = ERROR_CODE(0x80010039),
	SCE_ERROR_ERRNO_ENOSTR          = ERROR_CODE(0x8001003C),
	SCE_ERROR_ERRNO_ENODATA         = ERROR_CODE(0x8001003D),
	SCE_ERROR_ERRNO_ETIME           = ERROR_CODE(0x8001003E),
	SCE_ERROR_ERRNO_ENOSR           = ERROR_CODE(0x8001003F),
	SCE_ERROR_ERRNO_ENONET          = ERROR_CODE(0x80010040),
	SCE_ERROR_ERRNO_ENOPKG          = ERROR_CODE(0x80010041),
	SCE_ERROR_ERRNO_EREMOTE         = ERROR_CODE(0x80010042),
	SCE_ERROR_ERRNO_ENOLINK         = ERROR_CODE(0x80010043),
	SCE_ERROR_ERRNO_EADV            = ERROR_CODE(0x80010044),
	SCE_ERROR_ERRNO_ESRMNT          = ERROR_CODE(0x80010045),
	SCE_ERROR_ERRNO_ECOMM           = ERROR_CODE(0x80010046),
	SCE_ERROR_ERRNO_EPROTO          = ERROR_CODE(0x80010047),
	SCE_ERROR_ERRNO_EMULTIHOP       = ERROR_CODE(0x8001004A),
	SCE_ERROR_ERRNO_ELBIN           = ERROR_CODE(0x8001004B),
	SCE_ERROR_ERRNO_EDOTDOT         = ERROR_CODE(0x8001004C),
	SCE_ERROR_ERRNO_EBADMSG         = ERROR_CODE(0x8001004D),
	SCE_ERROR_ERRNO_EFTYPE          = ERROR_CODE(0x8001004F),
	SCE_ERROR_ERRNO_ENOTUNIQ        = ERROR_CODE(0x80010050),
	SCE_ERROR_ERRNO_EBADFD          = ERROR_CODE(0x80010051),
	SCE_ERROR_ERRNO_EREMCHG         = ERROR_CODE(0x80010052),
	SCE_ERROR_ERRNO_ELIBACC         = ERROR_CODE(0x80010053),
	SCE_ERROR_ERRNO_ELIBBAD         = ERROR_CODE(0x80010054),
	SCE_ERROR_ERRNO_ELIBSCN         = ERROR_CODE(0x80010055),
	SCE_ERROR_ERRNO_ELIBMAX         = ERROR_CODE(0x80010056),
	SCE_ERROR_ERRNO_ELIBEXEC        = ERROR_CODE(0x80010057),
	SCE_ERROR_ERRNO_ENOSYS          = ERROR_CODE(0x80010058),
	SCE_ERROR_ERRNO_ENMFILE         = ERROR_CODE(0x80010059),
	SCE_ERROR_ERRNO_ENOTEMPTY       = ERROR_CODE(0x8001005A),
	SCE_ERROR_ERRNO_ENAMETOOLONG    = ERROR_CODE(0x8001005B),
	SCE_ERROR_ERRNO_ELOOP           = ERROR_CODE(0x8001005C),
	SCE_ERROR_ERRNO_EOPNOTSUPP      = ERROR_CODE(0x8001005F),
	SCE_ERROR_ERRNO_EPFNOSUPPORT    = ERROR_CODE(0x80010060),
	SCE_ERROR_ERRNO_ECONNRESET      = ERROR_CODE(0x80010068),
	SCE_ERROR_ERRNO_ENOBUFS         = ERROR_CODE(0x80010069),
	SCE_ERROR_ERRNO_EAFNOSUPPORT    = ERROR_CODE(0x8001006A),
	SCE_ERROR_ERRNO_EPROTOTYPE      = ERROR_CODE(0x8001006B),
	SCE_ERROR_ERRNO_ENOTSOCK        = ERROR_CODE(0x8001006C),
	SCE_ERROR_ERRNO_ENOPROTOOPT     = ERROR_CODE(0x8001006D),
	SCE_ERROR_ERRNO_ESHUTDOWN       = ERROR_CODE(0x8001006E),
	SCE_ERROR_ERRNO_ECONNREFUSED    = ERROR_CODE(0x8001006F),
	SCE_ERROR_ERRNO_EADDRINUSE      = ERROR_CODE(0x80010070),
	SCE_ERROR_ERRNO_ECONNABORTED    = ERROR_CODE(0x80010071),
	SCE_ERROR_ERRNO_ENETUNREACH     = ERROR_CODE(0x80010072),
	SCE_ERROR_ERRNO_ENETDOWN        = ERROR_CODE(0x80010073),
	SCE_ERROR_ERRNO_ETIMEDOUT       = ERROR_CODE(0x80010074),
	SCE_ERROR_ERRNO_EHOSTDOWN       = ERROR_CODE(0x80010075),
	SCE_ERROR_ERRNO_EHOSTUNREACH    = ERROR_CODE(0x80010076),
	SCE_ERROR_ERRNO_EINPROGRESS     = ERROR_CODE(0x80010077),
	SCE_ERROR_ERRNO_EALREADY        = ERROR_CODE(0x80010078),
	SCE_ERROR_ERRNO_EDESTADDRREQ    = ERROR_CODE(0x80010079),
	SCE_ERROR_ERRNO_EMSGSIZE        = ERROR_CODE(0x8001007A),
	SCE_ERROR_ERRNO_EPROTONOSUPPORT = ERROR_CODE(0x8001007B),
	SCE_ERROR_ERRNO_ESOCKTNOSUPPORT = ERROR_CODE(0x8001007C),
	SCE_ERROR_ERRNO_EADDRNOTAVAIL   = ERROR_CODE(0x8001007D),
	SCE_ERROR_ERRNO_ENETRESET       = ERROR_CODE(0x8001007E),
	SCE_ERROR_ERRNO_EISCONN         = ERROR_CODE(0x8001007F),
	SCE_ERROR_ERRNO_ENOTCONN        = ERROR_CODE(0x80010080),
	SCE_ERROR_ERRNO_ETOOMANYREFS    = ERROR_CODE(0x80010081),
	SCE_ERROR_ERRNO_EPROCLIM        = ERROR_CODE(0x80010082),
	SCE_ERROR_ERRNO_EUSERS          = ERROR_CODE(0x80010083),
	SCE_ERROR_ERRNO_EDQUOT          = ERROR_CODE(0x80010084),
	SCE_ERROR_ERRNO_ESTALE          = ERROR_CODE(0x80010085),
	SCE_ERROR_ERRNO_ENOTSUP         = ERROR_CODE(0x80010086),
	SCE_ERROR_ERRNO_ENOMEDIUM       = ERROR_CODE(0x80010087),
	SCE_ERROR_ERRNO_ENOSHARE        = ERROR_CODE(0x80010088),
	SCE_ERROR_ERRNO_ECASECLASH      = ERROR_CODE(0x80010089),
	SCE_ERROR_ERRNO_EILSEQ          = ERROR_CODE(0x8001008A),
	SCE_ERROR_ERRNO_EOVERFLOW       = ERROR_CODE(0x8001008B),
	SCE_ERROR_ERRNO_ECANCELED       = ERROR_CODE(0x8001008C),
	SCE_ERROR_ERRNO_ENOTRECOVERABLE = ERROR_CODE(0x8001008D),
	SCE_ERROR_ERRNO_EOWNERDEAD      = ERROR_CODE(0x8001008E),
};

// Special return type signaling on errors
struct arm_error_code
{
	s32 value;

	// Print error message, error code is returned
	static s32 report(s32 error, const char* text);

	// Must be specialized for specific tag type T
	template<typename T>
	static const char* print(T code)
	{
		return nullptr;
	}

	template<typename T>
	s32 error_check(T code)
	{
		if (const auto text = print(code))
		{
			return report(code, text);
		}

		return code;
	}

	arm_error_code() = default;

	// General error check
	template<typename T, typename = std::enable_if_t<std::is_enum<T>::value>>
	arm_error_code(T value)
		: value(error_check(value))
	{
	}

	// Force error reporting with a message specified
	arm_error_code(s32 value, const char* text)
		: value(report(value, text))
	{
	}

	// Helper
	enum class not_an_error : s32 {};

	// Silence any error
	constexpr arm_error_code(not_an_error value)
		: value(static_cast<s32>(value))
	{
	}

	// Conversion
	constexpr operator s32() const
	{
		return value;
	}
};

// Helper macro for silencing possible error checks on returning arm_error_code values
#define NOT_AN_ERROR(...) static_cast<arm_error_code::not_an_error>(static_cast<s32>(__VA_ARGS__))

template<typename T, typename>
struct arm_gpr_cast_impl;

template<>
struct arm_gpr_cast_impl<arm_error_code, void>
{
	static inline u32 to(const arm_error_code& code)
	{
		return code;
	}

	static inline arm_error_code from(const u32 reg)
	{
		return NOT_AN_ERROR(reg);
	}
};

template<>
inline const char* arm_error_code::print(SceError error)
{
	switch (error)
	{
		STR_CASE(SCE_ERROR_ERRNO_EPERM);
		STR_CASE(SCE_ERROR_ERRNO_ENOENT);
		STR_CASE(SCE_ERROR_ERRNO_ESRCH);
		STR_CASE(SCE_ERROR_ERRNO_EINTR);
		STR_CASE(SCE_ERROR_ERRNO_EIO);
		STR_CASE(SCE_ERROR_ERRNO_ENXIO);
		STR_CASE(SCE_ERROR_ERRNO_E2BIG);
		STR_CASE(SCE_ERROR_ERRNO_ENOEXEC);
		STR_CASE(SCE_ERROR_ERRNO_EBADF);
		STR_CASE(SCE_ERROR_ERRNO_ECHILD);
		STR_CASE(SCE_ERROR_ERRNO_EAGAIN);
		STR_CASE(SCE_ERROR_ERRNO_ENOMEM);
		STR_CASE(SCE_ERROR_ERRNO_EACCES);
		STR_CASE(SCE_ERROR_ERRNO_EFAULT);
		STR_CASE(SCE_ERROR_ERRNO_ENOTBLK);
		STR_CASE(SCE_ERROR_ERRNO_EBUSY);
		STR_CASE(SCE_ERROR_ERRNO_EEXIST);
		STR_CASE(SCE_ERROR_ERRNO_EXDEV);
		STR_CASE(SCE_ERROR_ERRNO_ENODEV);
		STR_CASE(SCE_ERROR_ERRNO_ENOTDIR);
		STR_CASE(SCE_ERROR_ERRNO_EISDIR);
		STR_CASE(SCE_ERROR_ERRNO_EINVAL);
		STR_CASE(SCE_ERROR_ERRNO_ENFILE);
		STR_CASE(SCE_ERROR_ERRNO_EMFILE);
		STR_CASE(SCE_ERROR_ERRNO_ENOTTY);
		STR_CASE(SCE_ERROR_ERRNO_ETXTBSY);
		STR_CASE(SCE_ERROR_ERRNO_EFBIG);
		STR_CASE(SCE_ERROR_ERRNO_ENOSPC);
		STR_CASE(SCE_ERROR_ERRNO_ESPIPE);
		STR_CASE(SCE_ERROR_ERRNO_EROFS);
		STR_CASE(SCE_ERROR_ERRNO_EMLINK);
		STR_CASE(SCE_ERROR_ERRNO_EPIPE);
		STR_CASE(SCE_ERROR_ERRNO_EDOM);
		STR_CASE(SCE_ERROR_ERRNO_ERANGE);
		STR_CASE(SCE_ERROR_ERRNO_ENOMSG);
		STR_CASE(SCE_ERROR_ERRNO_EIDRM);
		STR_CASE(SCE_ERROR_ERRNO_ECHRNG);
		STR_CASE(SCE_ERROR_ERRNO_EL2NSYNC);
		STR_CASE(SCE_ERROR_ERRNO_EL3HLT);
		STR_CASE(SCE_ERROR_ERRNO_EL3RST);
		STR_CASE(SCE_ERROR_ERRNO_ELNRNG);
		STR_CASE(SCE_ERROR_ERRNO_EUNATCH);
		STR_CASE(SCE_ERROR_ERRNO_ENOCSI);
		STR_CASE(SCE_ERROR_ERRNO_EL2HLT);
		STR_CASE(SCE_ERROR_ERRNO_EDEADLK);
		STR_CASE(SCE_ERROR_ERRNO_ENOLCK);
		STR_CASE(SCE_ERROR_ERRNO_EFORMAT);
		STR_CASE(SCE_ERROR_ERRNO_EUNSUP);
		STR_CASE(SCE_ERROR_ERRNO_EBADE);
		STR_CASE(SCE_ERROR_ERRNO_EBADR);
		STR_CASE(SCE_ERROR_ERRNO_EXFULL);
		STR_CASE(SCE_ERROR_ERRNO_ENOANO);
		STR_CASE(SCE_ERROR_ERRNO_EBADRQC);
		STR_CASE(SCE_ERROR_ERRNO_EBADSLT);
		STR_CASE(SCE_ERROR_ERRNO_EDEADLOCK);
		STR_CASE(SCE_ERROR_ERRNO_EBFONT);
		STR_CASE(SCE_ERROR_ERRNO_ENOSTR);
		STR_CASE(SCE_ERROR_ERRNO_ENODATA);
		STR_CASE(SCE_ERROR_ERRNO_ETIME);
		STR_CASE(SCE_ERROR_ERRNO_ENOSR);
		STR_CASE(SCE_ERROR_ERRNO_ENONET);
		STR_CASE(SCE_ERROR_ERRNO_ENOPKG);
		STR_CASE(SCE_ERROR_ERRNO_EREMOTE);
		STR_CASE(SCE_ERROR_ERRNO_ENOLINK);
		STR_CASE(SCE_ERROR_ERRNO_EADV);
		STR_CASE(SCE_ERROR_ERRNO_ESRMNT);
		STR_CASE(SCE_ERROR_ERRNO_ECOMM);
		STR_CASE(SCE_ERROR_ERRNO_EPROTO);
		STR_CASE(SCE_ERROR_ERRNO_EMULTIHOP);
		STR_CASE(SCE_ERROR_ERRNO_ELBIN);
		STR_CASE(SCE_ERROR_ERRNO_EDOTDOT);
		STR_CASE(SCE_ERROR_ERRNO_EBADMSG);
		STR_CASE(SCE_ERROR_ERRNO_EFTYPE);
		STR_CASE(SCE_ERROR_ERRNO_ENOTUNIQ);
		STR_CASE(SCE_ERROR_ERRNO_EBADFD);
		STR_CASE(SCE_ERROR_ERRNO_EREMCHG);
		STR_CASE(SCE_ERROR_ERRNO_ELIBACC);
		STR_CASE(SCE_ERROR_ERRNO_ELIBBAD);
		STR_CASE(SCE_ERROR_ERRNO_ELIBSCN);
		STR_CASE(SCE_ERROR_ERRNO_ELIBMAX);
		STR_CASE(SCE_ERROR_ERRNO_ELIBEXEC);
		STR_CASE(SCE_ERROR_ERRNO_ENOSYS);
		STR_CASE(SCE_ERROR_ERRNO_ENMFILE);
		STR_CASE(SCE_ERROR_ERRNO_ENOTEMPTY);
		STR_CASE(SCE_ERROR_ERRNO_ENAMETOOLONG);
		STR_CASE(SCE_ERROR_ERRNO_ELOOP);
		STR_CASE(SCE_ERROR_ERRNO_EOPNOTSUPP);
		STR_CASE(SCE_ERROR_ERRNO_EPFNOSUPPORT);
		STR_CASE(SCE_ERROR_ERRNO_ECONNRESET);
		STR_CASE(SCE_ERROR_ERRNO_ENOBUFS);
		STR_CASE(SCE_ERROR_ERRNO_EAFNOSUPPORT);
		STR_CASE(SCE_ERROR_ERRNO_EPROTOTYPE);
		STR_CASE(SCE_ERROR_ERRNO_ENOTSOCK);
		STR_CASE(SCE_ERROR_ERRNO_ENOPROTOOPT);
		STR_CASE(SCE_ERROR_ERRNO_ESHUTDOWN);
		STR_CASE(SCE_ERROR_ERRNO_ECONNREFUSED);
		STR_CASE(SCE_ERROR_ERRNO_EADDRINUSE);
		STR_CASE(SCE_ERROR_ERRNO_ECONNABORTED);
		STR_CASE(SCE_ERROR_ERRNO_ENETUNREACH);
		STR_CASE(SCE_ERROR_ERRNO_ENETDOWN);
		STR_CASE(SCE_ERROR_ERRNO_ETIMEDOUT);
		STR_CASE(SCE_ERROR_ERRNO_EHOSTDOWN);
		STR_CASE(SCE_ERROR_ERRNO_EHOSTUNREACH);
		STR_CASE(SCE_ERROR_ERRNO_EINPROGRESS);
		STR_CASE(SCE_ERROR_ERRNO_EALREADY);
		STR_CASE(SCE_ERROR_ERRNO_EDESTADDRREQ);
		STR_CASE(SCE_ERROR_ERRNO_EMSGSIZE);
		STR_CASE(SCE_ERROR_ERRNO_EPROTONOSUPPORT);
		STR_CASE(SCE_ERROR_ERRNO_ESOCKTNOSUPPORT);
		STR_CASE(SCE_ERROR_ERRNO_EADDRNOTAVAIL);
		STR_CASE(SCE_ERROR_ERRNO_ENETRESET);
		STR_CASE(SCE_ERROR_ERRNO_EISCONN);
		STR_CASE(SCE_ERROR_ERRNO_ENOTCONN);
		STR_CASE(SCE_ERROR_ERRNO_ETOOMANYREFS);
		STR_CASE(SCE_ERROR_ERRNO_EPROCLIM);
		STR_CASE(SCE_ERROR_ERRNO_EUSERS);
		STR_CASE(SCE_ERROR_ERRNO_EDQUOT);
		STR_CASE(SCE_ERROR_ERRNO_ESTALE);
		STR_CASE(SCE_ERROR_ERRNO_ENOTSUP);
		STR_CASE(SCE_ERROR_ERRNO_ENOMEDIUM);
		STR_CASE(SCE_ERROR_ERRNO_ENOSHARE);
		STR_CASE(SCE_ERROR_ERRNO_ECASECLASH);
		STR_CASE(SCE_ERROR_ERRNO_EILSEQ);
		STR_CASE(SCE_ERROR_ERRNO_EOVERFLOW);
		STR_CASE(SCE_ERROR_ERRNO_ECANCELED);
		STR_CASE(SCE_ERROR_ERRNO_ENOTRECOVERABLE);
		STR_CASE(SCE_ERROR_ERRNO_EOWNERDEAD);
	}

	return nullptr;
}
