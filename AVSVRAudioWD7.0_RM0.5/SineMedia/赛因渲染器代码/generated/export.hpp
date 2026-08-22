
#ifndef RENDER_EXPORT_H
#define RENDER_EXPORT_H

#ifdef RENDER_STATIC_DEFINE
#  define RENDER_EXPORT
#  define RENDER_NO_EXPORT
#else
#  ifndef RENDER_EXPORT
#    ifdef render_EXPORTS
        /* We are building this library */
#      define RENDER_EXPORT 
#    else
        /* We are using this library */
#      define RENDER_EXPORT 
#    endif
#  endif

#  ifndef RENDER_NO_EXPORT
#    define RENDER_NO_EXPORT 
#  endif
#endif

#ifndef RENDER_DEPRECATED
#  define RENDER_DEPRECATED __declspec(deprecated)
#endif

#ifndef RENDER_DEPRECATED_EXPORT
#  define RENDER_DEPRECATED_EXPORT RENDER_EXPORT RENDER_DEPRECATED
#endif

#ifndef RENDER_DEPRECATED_NO_EXPORT
#  define RENDER_DEPRECATED_NO_EXPORT RENDER_NO_EXPORT RENDER_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef RENDER_NO_DEPRECATED
#    define RENDER_NO_DEPRECATED
#  endif
#endif

#endif /* RENDER_EXPORT_H */
