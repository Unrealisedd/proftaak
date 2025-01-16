using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace BottleReturnApi.Models
{
    [Table("logs")]
    public class LogEntry
    {
        [Key]
        public int ID { get; set; }
        public DateTime datumtijd { get; set; }
        public string level { get; set; }
        public string bericht { get; set; }
        public string exception { get; set; }
        public string donatie_ID { get; set; }
    }
}
