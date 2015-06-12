#ifndef DCE_IF_H
#define DCE_IF_H


#ifdef __cplusplus
extern "C" {
#endif

/**
 * \param
 * \return On success, if_nametoindex() returns the index number of the network interface;
            on error, 0 is returned and  errno  is  set appropriately.
 */
unsigned dce_if_nametoindex (const char *ifname);

/**
 *
 */
char * dce_if_indextoname (unsigned ifindex, char *ifname);


#ifdef __cplusplus
}
#endif

#endif /* DCE_IF_H */
