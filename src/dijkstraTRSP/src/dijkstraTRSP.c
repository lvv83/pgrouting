/*PGR-GNU*****************************************************************
File: dijkstraTRSP.c

Generated with Template by:
Copyright (c) 2015 pgRouting developers
Mail: project@pgrouting.org

Function's developer:
Copyright (c) 2017 Vidhan Jain
Mail: vidhanj1307@gmail.com

------

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

********************************************************************PGR-GNU*/

/** @file dijkstraTRSP.c
 * @brief Connecting code with postgres.
 *
 * This file is fully documented for understanding
 *  how the postgres connectinon works
 *
 * TODO Remove unnecessary comments before submiting the function.
 * some comments are in form of PGR_DBG message
 */

/**
 *  postgres_connection.h
 *
 *  - should always be first in the C code
 */
#include "c_common/postgres_connection.h"


/* for macro PGR_DBG */
#include "c_common/debug_macro.h"
/* for pgr_global_report */
#include "c_common/e_report.h"
/* for time_msg & clock */
#include "c_common/time_msg.h"
/* for functions to get edges information */
#include "c_common/edges_input.h"
#include "c_common/restrict_input.h"

#include "drivers/dijkstraTRSP/dijkstraTRSP_driver.h"  // the link to the C++ code of the function

PGDLLEXPORT Datum dijkstraTRSP(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(dijkstraTRSP);


/******************************************************************************/
/*                          MODIFY AS NEEDED                                  */
static
void
process(
        char* edges_sql,
        char *restrictions_sql,
        int64_t start_vid,
        int64_t end_vid,
        bool directed,
        bool only_cost,
        bool strict,
        General_path_element_t **result_tuples,
        size_t *result_count) {
    pgr_SPI_connect();
    /*
     *  https://www.postgresql.org/docs/current/static/spi-spi-connect.html
     */
    (*result_tuples) = NULL;
    (*result_count) = 0;
    PGR_DBG("\n\n\n\n\n\nEdge query: %s\n", edges_sql);
    PGR_DBG("Restrictions query: %s\n", restrictions_sql);
    PGR_DBG("source: %lu | destination: %lu\n\n", start_vid, end_vid);

    PGR_DBG("Load data");
    pgr_edge_t *edges = NULL;
    size_t total_edges = 0;

    if (start_vid == end_vid) {
        /*
         * https://www.postgresql.org/docs/current/static/spi-spi-finish.html
         */
        pgr_SPI_finish();
        return;
    }

    pgr_get_edges(edges_sql, &edges, &total_edges);
    PGR_DBG("Total %ld edges in query:", total_edges);

    PGR_DBG("Load restrictions");
    Restrict_t *restrictions = NULL;
    size_t total_restrictions = 0;

    pgr_get_restriction_data(restrictions_sql, &restrictions,
        &total_restrictions);

#if 1
    size_t i = 0;
    while(i < total_restrictions) {
        PGR_DBG("id: %ld cost: %lf", restrictions[i].id, restrictions[i].cost);
        int j = 0;
        while(restrictions[i].restricted_edges[j] != -1) {
            PGR_DBG("%ld ", restrictions[i].restricted_edges[j]);
            j++;
        }
        PGR_DBG("\n");
        i++;
    }
#endif

    if (total_edges == 0) {
        PGR_DBG("No edges found");
        pgr_SPI_finish();
        return;
    }

    PGR_DBG("Starting processing");
    clock_t start_t = clock();
    char *log_msg = NULL;
    char *notice_msg = NULL;
    char *err_msg = NULL;
    do_pgr_dijkstraTRSP(
            edges,
            total_edges,
            restrictions,
            total_restrictions,
            start_vid,
            end_vid,
            directed,
            only_cost,
            strict,
            result_tuples,
            result_count,
            &log_msg,
            &notice_msg,
            &err_msg);

    time_msg(" processing pgr_dijkstraTRSP", start_t, clock());
    PGR_DBG("Returning %ld tuples", *result_count);

    if (err_msg) {
        if (*result_tuples) pfree(*result_tuples);
    }
    pgr_global_report(log_msg, notice_msg, err_msg);
    if (edges) pfree(edges);
    if (log_msg) pfree(log_msg);
    if (notice_msg) pfree(notice_msg);
    if (err_msg) pfree(err_msg);
    if (restrictions) pfree(restrictions);
    pgr_SPI_finish();
}
/*                                                                            */
/******************************************************************************/

PGDLLEXPORT Datum dijkstraTRSP(PG_FUNCTION_ARGS) {
    FuncCallContext     *funcctx;
    TupleDesc           tuple_desc;

    /**************************************************************************/
    /*                          MODIFY AS NEEDED                              */
    /*                                                                        */
    General_path_element_t  *result_tuples = NULL;
    size_t result_count = 0;
    /*                                                                        */
    /**************************************************************************/

    if (SRF_IS_FIRSTCALL()) {
        MemoryContext   oldcontext;
        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);


        /**********************************************************************/
        /*                          MODIFY AS NEEDED                          */
        /*
           TEXT,
    TEXT,
    BIGINT,
    BIGINT,
    directed BOOLEAN DEFAULT true,
    only_cost BOOLEAN DEFAULT false,
    strict BOOLEAN DEFAULT false
         **********************************************************************/


        PGR_DBG("Calling process");
        process(
                text_to_cstring(PG_GETARG_TEXT_P(0)),
                text_to_cstring(PG_GETARG_TEXT_P(1)),
                PG_GETARG_INT64(2),
                PG_GETARG_INT64(3),
                PG_GETARG_BOOL(4),
                PG_GETARG_BOOL(5),
                PG_GETARG_BOOL(6),
                &result_tuples,
                &result_count);


        /*                                                                    */
        /**********************************************************************/

#if PGSQL_VERSION > 94
        funcctx->max_calls = result_count;
#else
        funcctx->max_calls = (uint32_t)result_count;
#endif
        funcctx->user_fctx = result_tuples;
        if (get_call_result_type(fcinfo, NULL, &tuple_desc)
                != TYPEFUNC_COMPOSITE) {
            ereport(ERROR,
                    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                     errmsg("function returning record called in context "
                         "that cannot accept type record")));
        }

        funcctx->tuple_desc = tuple_desc;
        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    tuple_desc = funcctx->tuple_desc;
    result_tuples = (General_path_element_t*) funcctx->user_fctx;

    if (funcctx->call_cntr < funcctx->max_calls) {
        HeapTuple    tuple;
        Datum        result;
        Datum        *values;
        bool*        nulls;

        /**********************************************************************/
        /*                          MODIFY AS NEEDED                          */
        /*
               OUT seq INTEGER,
    OUT path_seq INTEGER,
    OUT node BIGINT,
    OUT edge BIGINT,
    OUT cost FLOAT,
    OUT agg_cost FLOAT
         ***********************************************************************/

        values = palloc(6 * sizeof(Datum));
        nulls = palloc(6 * sizeof(bool));


        size_t i;
        for (i = 0; i < 6; ++i) {
            nulls[i] = false;
        }

        // postgres starts counting from 1
        values[0] = Int32GetDatum(funcctx->call_cntr + 1);
        values[1] = Int32GetDatum(result_tuples[funcctx->call_cntr].seq);
        values[2] = Int64GetDatum(result_tuples[funcctx->call_cntr].node);
        values[3] = Int64GetDatum(result_tuples[funcctx->call_cntr].edge);
        values[4] = Float8GetDatum(result_tuples[funcctx->call_cntr].cost);
        values[5] = Float8GetDatum(result_tuples[funcctx->call_cntr].agg_cost);
        /**********************************************************************/

        tuple = heap_form_tuple(tuple_desc, values, nulls);
        result = HeapTupleGetDatum(tuple);
        SRF_RETURN_NEXT(funcctx, result);
    } else {
        /**********************************************************************/
        /*                          MODIFY AS NEEDED                          */

        PGR_DBG("Clean up code");

        /**********************************************************************/

        SRF_RETURN_DONE(funcctx);
    }
}
