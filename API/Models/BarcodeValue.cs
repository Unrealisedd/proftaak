using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace BottleReturnApi.Models
{
    [Table("waarde")]
    public class BarcodeValue
    {
        [Key]
        public string barcode { get; set; }
        public decimal bedrag { get; set; }
    }
}
