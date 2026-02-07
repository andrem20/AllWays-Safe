const express = require("express");

module.exports = (supabase) => {
    const router = express.Router();

    router.post("/data/:table", async (req, res) => {
        try {
            const { table } = req.params;
            const payload = req.body;

            const allowedTables = [
                "p_semaphore_pedestrian",
                "emergencyvehicle"
            ];

            if (!allowedTables.includes(table)) {
                return res.status(400).json({
                    error: 400,
                    detail: "Table not allowed"
                });
            }

            /* =========================
               PSEM_PEDESTRIAN
            ========================= */
            if (table === "p_semaphore_pedestrian") {
                const requiredFields = [
                    "psem_id",
                    "pedestrianCC_id",
                    "timestamp"
                ];

                for (const f of requiredFields) {
                    if (!(f in payload)) {
                        return res.status(400).json({
                            error: 400,
                            detail: `Missing field ${f}`
                        });
                    }
                }
            }

            /* =========================
               EMERGENCY VEHICLE
            ========================= */
            if (table === "emergencyvehicle") {
                const requiredFields = [
                    "tmcid",
                    "controlbox_id",
                    "licenseplate",
                    "timestamp",
                    "origin",
                    "destination",
                    "priority_level"
                ];

                for (const f of requiredFields) {
                    if (!(f in payload)) {
                        return res.status(400).json({
                            error: 400,
                            detail: `Missing field ${f}`
                        });
                    }
                }
            }

            const { data, error } = await supabase
                .from(table)
                .insert(payload)
                .select();

            if (error) {
                return res.status(500).json({
                    error: 500,
                    detail: error.message
                });
            }

            return res.status(201).json(data[0]);

        } catch (err) {
            return res.status(500).json({
                error: 500,
                detail: err.message
            });
        }
    });

    return router;
};
