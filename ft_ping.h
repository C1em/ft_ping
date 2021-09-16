/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ping.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/31 12:46:55 by coremart          #+#    #+#             */
/*   Updated: 2021/09/14 15:56:53 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PING_H
# define FT_PING_H

#define DEST_IP				"8.8.8.8"

#ifdef __linux__
	#define IS_LINUX (1)
#else
	#define IS_LINUX (0)
#endif

struct tv32 {
	u_int32_t tv32_sec;
	u_int32_t tv32_usec;
};

# define TV_LEN (sizeof(struct tv32))

#endif
