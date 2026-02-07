const express = require("express");

module.exports = (supabase) => {
    const router = express.Router();

    router.get("/data/:table", handler);
    router.get("/data/:table/:field/:value", handler);

    return router;

    async function handler(req, res) {
        try {
            const { table, field, value } = req.params;

            const allowedTables = [
                "t_semaphore",
                "controlbox",
                "p_semaphore",
                "destinations",
                "tmc",
                "pedestrian"
            ];

            if (!allowedTables.includes(table)) {
                return res.status(400).json({ error: 400, detail: "Table not allowed" });
            }

            const allowedFields = ["tmcid", "controlboxid", "physicaltag_id", "id"];

            if (field && !allowedFields.includes(field)) {
                return res.status(400).json({ error: 400, detail: "Field not allowed" });
            }

            /* =========================
            T_SEMAPHORE WITH DESTINATIONS
            ========================= */
            if (table === "t_semaphore") {
                let query = supabase.from("t_semaphore").select("*");
                if (field && value) query = query.eq(field, value);

                const { data: tsemData, error } = await query;

                if (error || !tsemData || tsemData.length === 0) {
                    return res.status(404).json({ error: 404, detail: "TSEM not found" });
                }

                const result = await Promise.all(
                    tsemData.map(async (tsem) => {
                        const { data: destData } = await supabase
                            .from("destinations")
                            .select("destinationnumber")
                            .eq("tsemid", tsem.id);

                        return {
                            ...tsem,
                            destinations: destData ? destData.map(d => d.destinationnumber) : []
                        };
                    })
                );

                return res.status(200).json(result);
            }

            /* =========================
               PEDESTRIAN- ask for physicalTag ID
            ========================= */
            if (table === "pedestrian" && field === "physicaltag_id") {
                const { data, error } = await supabase
                    .from("pedestrian")
                    .select("cc_id")
                    .eq("physicaltag_id", value)
                    .limit(1);

                if (error) {
                    return res.status(500).json({ error: error.message });
                }

                return res.status(200).json({
                    found: data && data.length > 0
                });
            }

            /* =========================
            ask for everthing froma table
            ========================= */
            let query = supabase.from(table).select("*");
            if (field && value) query = query.eq(field, value);

            const { data, error } = await query;

            if (error) {
                return res.status(500).json({ error: 500, detail: error.message });
            }

            return res.status(200).json(data);

        } catch (err) {
            return res.status(500).json({ error: 500, detail: err.message });
        }
    }
};
