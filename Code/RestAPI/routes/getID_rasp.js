const express = require("express");

module.exports = (supabase) => {
    const router = express.Router();

    router.get("/id/:table/:identifier", async (req, res) => {
        try {
            const { table, identifier } = req.params;

            const TABLE_CONFIG = {
                p_semaphore: { identifierField: "location", idField: "id" },
                controlbox:  { identifierField: "name", idField: "id" },
                tmc:         { identifierField: "username", idField: "id" },
                pedestrian:  { identifierField: "physicaltag_id", idField: "cc_id" }
            };

            const config = TABLE_CONFIG[table];

            if (!config) {
                return res.status(400).json({
                    error: 400,
                    detail: "Table not allowed"
                });
            }

            const { identifierField, idField } = config;

            const { data, error } = await supabase
                .from(table)
                .select(idField)
                .eq(identifierField, identifier)
                .limit(1);

            if (error) {
                return res.status(500).json({
                    error: 500,
                    detail: error.message
                });
            }

            if (!data || data.length === 0) {
                return res.status(404).json({
                    error: 404,
                    detail: "Record not found"
                });
            }
            //returns ID
            return res.status(200).json({
                id: data[0][idField]
            });

        } catch (err) {
            return res.status(500).json({
                error: 500,
                detail: err.message
            });
        }
    });

    return router;
};
